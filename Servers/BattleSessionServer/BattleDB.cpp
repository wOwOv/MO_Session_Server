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
}

BattleDB::BattleDB(DBConnector& db):_db(db)
{}

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
	const int redWin = result._winnerTeam == 0 ? 1 : 0;
	const int blueWin = result._winnerTeam == 1 ? 1 : 0;

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
	const int redWin = result._winnerTeam == 0 ? 1 : 0;
	const int redLoss = result._winnerTeam == 0 ? 0 : 1;

	const int blueWin = result._winnerTeam == 1 ? 1 : 0;
	const int blueLoss = result._winnerTeam == 1 ? 0 : 1;

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
