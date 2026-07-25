#include "BattleDB.h"

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
}

BattleDB::BattleDB(DBConnector& db):_db(db)
{}

bool BattleDB::SaveBattleResult(const BattleResult & result)
{
	if (!_db.BeginTransaction())
	{
		const DBErrorInfo error = _db.GetLastError();

		LOG(L"Database", LVERROR,L"SaveBattleResult failed. match_id=%lld stage=Begin category=%s mysql_error=%u",
			result._matchID,GetDBErrorCategoryName(error.category),	error.mysqlError);

		return false;
	}

	if (!InsertBattleHistory(result))
	{
		const DBErrorInfo originalError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		LOG(L"Database", LVERROR,L"SaveBattleResult failed. match_id=%lld stage=BattleHistory category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u",
			result._matchID,GetDBErrorCategoryName(originalError.category),originalError.mysqlError,rollbackSucceeded,rollbackSucceeded ? 0 : rollbackError.mysqlError);

		return false;
	}

	if (!InsertPlayerBattleRecords(result))
	{
		const DBErrorInfo originalError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		LOG(L"Database", LVERROR, L"SaveBattleResult failed. match_id=%lld stage=PlayerBattleRecord category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u",
			result._matchID, GetDBErrorCategoryName(originalError.category), originalError.mysqlError, rollbackSucceeded, rollbackSucceeded ? 0 : rollbackError.mysqlError);

		return false;
	}

	if (!UpsertPlayerBattleStats(result))
	{
		const DBErrorInfo originalError = _db.GetLastError();

		const bool rollbackSucceeded = _db.Rollback();
		const DBErrorInfo rollbackError = _db.GetLastError();

		LOG(L"Database", LVERROR, L"SaveBattleResult failed. match_id=%lld stage=PlayerBattleStat category=%s mysql_error=%u rollback=%d rollback_mysql_error=%u",
			result._matchID, GetDBErrorCategoryName(originalError.category), originalError.mysqlError, rollbackSucceeded, rollbackSucceeded ? 0 : rollbackError.mysqlError);

		return false;
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

		return false;
	}

	LOG(L"Database", LVSYSTEM, L"SaveBattleResult success. match_id=%llu",
		result._matchID);

	return true;
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
