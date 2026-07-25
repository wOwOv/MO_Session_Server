#pragma once

#include "FighterStructure.h"
#include "DBConnector.h"
#include <cstdint>

enum class DBSaveStage : std::uint8_t
{
    None,
    BeginTransaction,
    BattleHistory,
    PlayerBattleRecord,
    PlayerBattleStat,
    Commit,
};

struct DBSaveResult
{
    bool succeeded = false;
    DBSaveStage stage = DBSaveStage::None;
    DBErrorInfo error{};
    bool rollbackAttempted = false;
    bool rollbackSucceeded = false;
    bool commitOutcomeUnknown = false;
};

class BattleDB
{
public:
	explicit BattleDB(DBConnector& db);
    DBSaveResult SaveBattleResult(const BattleResult& result);

private:
	bool InsertBattleHistory(const BattleResult& result);
	bool InsertPlayerBattleRecords(const BattleResult& result);
	bool UpsertPlayerBattleStats(const BattleResult& result);

private:
	DBConnector& _db;
};

