#pragma once
#include "Contents.h"
#include <unordered_map>
#include "FighterStructure.h"

class MatchContents:public Contents
{
public:
	MatchContents();
	~MatchContents();

	virtual void OnEnter(__int64 sessionID, void* extra) override;
	virtual void OnLeave(__int64 sessionID, void* extra) override;
	virtual void OnUpdate() override;

private:
	std::unordered_map<SessionID, Player* > _playerMap;

};

class FightContents :public Contents
{
public:
	FightContents();
	~FightContents();

	virtual void OnEnter(__int64 sessionID, void* extra) override;
	virtual void OnLeave(__int64 sessionID, void* extra)override;
	virtual void OnUpdate() override;

	void Init(__int64 matchID);
	void Clear();
	bool CheckGameEnd();

private:
	std::unordered_map<SessionID, Player* > _playerMap;
	__int64 _matchID = 0;
	__int32 _matched = 0;
	__int32 _redCount = 0;
	__int32 _blueCount = 0;
	DWORD _oldTick;

	bool _end = 1;//1이면 끄기 진행 전 0이면 끄기 진행 중
	friend class StubForFight;
};