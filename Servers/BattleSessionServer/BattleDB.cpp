#include "BattleDB.h"
#include <chrono>
#include <thread>

namespace
{
	const wchar_t* BATTLE_HISTORY_SAVE_QUERY = L"\
		INSERT INTO battle_history\
		(\
			match_id,\
			winner_team\
		)\
		VALUES\
		(\
			%I64u,\
			%d\
		);";

	const wchar_t* PLAYER_BATTLE_RECORD_SAVE_QUERY = L"\
		INSERT INTO player_battle_record\
		(\
			match_id,\
			account_no,\
			team,\
			slot_no,\
			is_win\
		)\
		VALUES\
		(%I64u, %I64u, %d, %d, %d),\
		(%I64u, %I64u, %d, %d, %d),\
		(%I64u, %I64u, %d, %d, %d),\
		(%I64u, %I64u, %d, %d, %d),\
		(%I64u, %I64u, %d, %d, %d),\
		(%I64u, %I64u, %d, %d, %d);";

	const wchar_t* PLAYER_BATTLE_STAT_SAVE_QUERY = L"\
		INSERT INTO player_battle_stat\
		(\
			account_no,\
			total_battle_count,\
			win_count,\
			loss_count\
		)\
		VALUES\
		(%I64u, 1, %d, %d),\
		(%I64u, 1, %d, %d),\
		(%I64u, 1, %d, %d),\
		(%I64u, 1, %d, %d),\
		(%I64u, 1, %d, %d),\
		(%I64u, 1, %d, %d)\
		ON DUPLICATE KEY UPDATE\
			total_battle_count = total_battle_count + VALUES(total_battle_count),\
			win_count = win_count + VALUES(win_count),\
			loss_count = loss_count + VALUES(loss_count);";

	const wchar_t* GetDBErrorCategoryName(DBErrorCategory category)
	{
		switch (category)
		{
		case DBErrorCategory::DuplicateKey:
			return L"DuplicateKey";
		case DBErrorCategory::Deadlock:
			return L"Deadlock";
		case DBErrorCategory::LockTimeout:
			return L"LockTimeout";
		case DBErrorCategory::ConnectionLost:
			return L"ConnectionLost";
		case DBErrorCategory::QueryFormatError:
			return L"QueryFormatError";
		case DBErrorCategory::Unknown:
			return L"Unknown";
		default:
			return L"None";
		}
	}

	bool IsRetryableSaveError(const DBSaveResult& saveResult)
	{
		if (saveResult.commitOutcomeUnknown)
		{
			return false;
		}

		if (saveResult.rollbackAttempted && !saveResult.rollbackSucceeded)
		{
			return false;
		}

		if (saveResult.stage == DBSaveStage::Commit)
		{
			return false;
		}

		return saveResult.error.category == DBErrorCategory::Deadlock ||
			saveResult.error.category == DBErrorCategory::LockTimeout;
	}
}

BattleDB::BattleDB(DBConnector& db):_db(db)
{}

DBSaveResult BattleDB::SaveBattleResult(const BattleResult& result)
{
	for (int attempt = 1; attempt <= kMaxSaveAttempts; ++attempt)
	{
		DBSaveResult saveResult = SaveBattleResultOnce(result);
		saveResult.attemptCount = static_cast<std::uint8_t>(attempt);

		if (saveResult.succeeded)
		{
			return saveResult;
		}

		if (!IsRetryableSaveError(saveResult))
		{
			return saveResult;
		}

		if (attempt == kMaxSaveAttempts)
		{
			saveResult.retryExhausted = true;
			return saveResult;
		}

		const int backoffMs = kRetryBackoffMs * attempt;

		LOG(
			L"Database",
			LVERROR,
			L"Retrying battle result save. match_id=%lld attempt=%d next_attempt=%d "
			L"category=%s mysql_error=%u backoff_ms=%d",
			result._matchID,
			attempt,
			attempt + 1,
			GetDBErrorCategoryName(saveResult.error.category),
			saveResult.error.mysqlError,
			backoffMs);

		std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
	}

	return {};
}

DBSaveResult BattleDB::SaveBattleResultOnce(const BattleResult& result)
{
	if (!_db.BeginTransaction())
	{
		const DBErrorInfo error = _db.GetLastError();

		LOG(L"Database", LVERROR,L"SaveBattleResult failed. match_id=%lld stage=Begin category=%s mysql_error=%u",
			result._matchID,GetDBErrorCategoryName(error.category),	error.mysqlError);

		DBSaveResult saveResult;
		saveResult.succeeded = false;
		saveResult.stage = DBSaveStage::BeginTransaction;
		saveResult.error = error;
		return saveResult;
	}

	if (!InsertBattleHistory(result))
	{
		const DBErrorInfo originalError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		LOG(L"Database", LVERROR,L"SaveBattleResult failed. match_id=%lld stage=BattleHistory category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u",
			result._matchID,GetDBErrorCategoryName(originalError.category),originalError.mysqlError,rollbackSucceeded,rollbackSucceeded ? 0u : rollbackError.mysqlError);

		DBSaveResult saveResult;
		saveResult.stage = DBSaveStage::BattleHistory;
		saveResult.error = originalError;
		saveResult.rollbackAttempted = true;
		saveResult.rollbackSucceeded = rollbackSucceeded;
		return saveResult;
	}

	if (!InsertPlayerBattleRecords(result))
	{
		const DBErrorInfo originalError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		LOG(L"Database", LVERROR, L"SaveBattleResult failed. match_id=%lld stage=PlayerBattleRecord category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u",
			result._matchID, GetDBErrorCategoryName(originalError.category), originalError.mysqlError, rollbackSucceeded, rollbackSucceeded ? 0u : rollbackError.mysqlError);
		
		DBSaveResult saveResult;
		saveResult.stage = DBSaveStage::PlayerBattleRecord;
		saveResult.error = originalError;
		saveResult.rollbackAttempted = true;
		saveResult.rollbackSucceeded = rollbackSucceeded;
		return saveResult;
	}

	if (!UpsertPlayerBattleStats(result))
	{
		const DBErrorInfo originalError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		LOG(L"Database", LVERROR, L"SaveBattleResult failed. match_id=%lld stage=PlayerBattleStat category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u",
			result._matchID, GetDBErrorCategoryName(originalError.category), originalError.mysqlError, rollbackSucceeded, rollbackSucceeded ? 0u : rollbackError.mysqlError);

		DBSaveResult saveResult;
		saveResult.stage = DBSaveStage::PlayerBattleStat;
		saveResult.error = originalError;
		saveResult.rollbackAttempted = true;
		saveResult.rollbackSucceeded = rollbackSucceeded;
		return saveResult;
	}

	if (!_db.Commit())
	{
		const DBErrorInfo commitError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		const bool commitOutcomeUnknown =commitError.category == DBErrorCategory::ConnectionLost || !rollbackSucceeded;
		const wchar_t* commitOutcome = commitOutcomeUnknown ? L"Unknown" : L"RolledBack";

		LOG(L"Database", LVERROR,L"SaveBattleResult commit failed. match_id=%lld category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u commit_outcome=%s",
			result._matchID,GetDBErrorCategoryName(commitError.category),commitError.mysqlError,rollbackSucceeded,rollbackSucceeded ? 0u : rollbackError.mysqlError,commitOutcome);

		DBSaveResult saveResult;
		saveResult.succeeded = false;
		saveResult.stage = DBSaveStage::Commit;
		saveResult.error = commitError;
		saveResult.rollbackAttempted = true;
		saveResult.rollbackSucceeded = rollbackSucceeded;
		saveResult.commitOutcomeUnknown = commitOutcomeUnknown;
		return saveResult;
	}

	LOG(L"Database", LVSYSTEM, L"SaveBattleResult success. match_id=%llu",
		result._matchID);

	DBSaveResult saveResult;
	saveResult.succeeded = true;
	return saveResult;
}

bool BattleDB::InsertBattleHistory(const BattleResult& result)
{
	return _db.QuerySave(
		BATTLE_HISTORY_SAVE_QUERY,
		result._matchID,
		result._winnerTeam
	);
}

bool BattleDB::InsertPlayerBattleRecords(const BattleResult& result)
{
	const int redWin = result._winnerTeam == 1 ? 1 : 0;
	const int blueWin = result._winnerTeam == 2 ? 1 : 0;

	return _db.QuerySave(
		PLAYER_BATTLE_RECORD_SAVE_QUERY,

		result._matchID, result._red[0], 0, 0, redWin,
		result._matchID, result._red[1], 0, 1, redWin,
		result._matchID, result._red[2], 0, 2, redWin,

		result._matchID, result._blue[0], 1, 0, blueWin,
		result._matchID, result._blue[1], 1, 1, blueWin,
		result._matchID, result._blue[2], 1, 2, blueWin
	);
}

bool BattleDB::UpsertPlayerBattleStats(const BattleResult& result)
{
	const int redWin = result._winnerTeam == 1 ? 1 : 0;
	const int redLoss = result._winnerTeam == 1 ? 0 : 1;

	const int blueWin = result._winnerTeam == 2 ? 1 : 0;
	const int blueLoss = result._winnerTeam == 2 ? 0 : 1;

	return _db.QuerySave(
		PLAYER_BATTLE_STAT_SAVE_QUERY,

		result._red[0], redWin, redLoss,
		result._red[1], redWin, redLoss,
		result._red[2], redWin, redLoss,

		result._blue[0], blueWin, blueLoss,
		result._blue[1], blueWin, blueLoss,
		result._blue[2], blueWin, blueLoss
	);
}
