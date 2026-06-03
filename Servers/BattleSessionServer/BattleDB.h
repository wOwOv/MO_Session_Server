#pragma once

#include "FighterStructure.h"
#include "DBConnector.h"


class BattleDB
{
public:
	explicit BattleDB(DBConnector& db);
	bool SaveBattleResult(const BattleResult& result);

private:
	bool InsertBattleHistory(const BattleResult& result);
	bool InsertPlayerBattleRecords(const BattleResult& result);
	bool UpsertPlayerBattleStats(const BattleResult& result);

private:
	DBConnector& _db;
};

