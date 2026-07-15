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
	virtual void OnShutDown() override;

private:
	void DisconnectAllPlayer();
	bool CheckStopRequested();

private:
	std::atomic<bool> _stopRequested = false;
	std::unordered_map<SessionID, Player* > _playerMap;// non-owning; Player lifetime is managed by FighterServer::_playerPool

};

class FightContents :public Contents
{
public:
	FightContents();
	~FightContents();

	virtual void OnEnter(__int64 sessionID, void* extra) override;
	virtual void OnLeave(__int64 sessionID, void* extra)override;
	virtual void OnUpdate() override;
	virtual void OnShutDown() override;

	void Init(__int32 contentsNum, __int64 matchID, const Group& group);
	void Clear();
	bool CheckGameEnd();

private:
	std::unordered_map<SessionID, Player* > _playerMap;// non-owning; Player lifetime is managed by FighterServer::_playerPool
	SessionID _red[3];
	SessionID _blue[3];
	__int64 _matchID = 0;
	__int32 _matched = 0;
	__int32 _redCount = 0;
	__int32 _blueCount = 0;
	__int32 _winLoss = 0;		//0: none, 1: red win, 2: blue win
	DWORD _oldTick;

	bool _end = 1;//1이면 끄기 진행 전 0이면 끄기 진행 중
	friend class StubForFight;
};