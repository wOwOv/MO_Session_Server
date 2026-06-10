#pragma once
#include "ContentsServer.h"
#include "FighterStructure.h"
#include "TlsMemoryPool.h"
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>
#include <thread>
#include "MemoryPool.h"
#include "LockFreeQueue(CAS).h"
#include "FighterContents.h"
#include <queue>
#include "MatchIDGenerator.h"

class FighterServer:public ContentsServer
{
	friend class MatchContents;
	friend class FightContents;

public:
	FighterServer();
	~FighterServer();

	virtual bool OnConnectionRequest(SOCKADDR_IN* clientaddr) override;
	virtual void OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID) override;
	virtual void OnRelease(__int64 sessionID, __int32 contentsnum)override;
	virtual void OnUnusual(__int64 sessionID, SOCKADDR_IN clientaddr) override;
	virtual void OnSecond() override;

	int GetFightPoolCapacity();
	int GetFightPoolUsingCount();
	int GetControlQSize();
	int GetPlayerCount();
	int GetPlayerPoolCapacity();
	int GetPlayerPoolUsingCount();
	int GetControlPoolCapacity();
	int GetControlPoolUsingCount();
	
	void StopDBThread();

	__int64 CreateMatchID();

	void RequestSaveBattleResult(const BattleResult& result);

	std::shared_mutex& GetPlayerLock();

private:
	static unsigned __stdcall CtrlThread(LPVOID arg);
	static unsigned __stdcall DBThread(LPVOID arg);

private:
	void PushDBRequest(const DBRequest& request);
	bool WaitAndPopDBRequest(DBRequest& outrequest);

private:
	struct SockAddrInHash {
		std::size_t operator()(const SOCKADDR_IN& addr) const {
			return std::hash<uint32_t>()(addr.sin_addr.S_un.S_addr)
				^ std::hash<uint16_t>()(addr.sin_port);
		}
	};
	struct SockAddrInEqual {
		bool operator()(const SOCKADDR_IN& a, const SOCKADDR_IN& b) const {
			return a.sin_addr.S_un.S_addr == b.sin_addr.S_un.S_addr
				&& a.sin_port == b.sin_port;
		}
	};

private:
	//제어스레드
	std::thread _CtrlThread;
	LFQueue<Control*> _ctrlQ;
	std::condition_variable _ctrlCv;
	std::mutex _ctrlMtx;
	MemoryPool<FightContents> _fightPool;

	MatchIDGenerator _matchIDGenerator;

	//DB저장스레드
	std::thread _DBThread;
	std::queue<DBRequest> _dbQ;
	std::condition_variable _dbCv;
	std::mutex _dbMtx;
	bool _dbThreadRun;


	std::unordered_set<SOCKADDR_IN, SockAddrInHash, SockAddrInEqual> _banSet;
	std::shared_mutex _banMutex;

	std::unordered_map<SessionID,Player*> _playerMap;
	std::shared_mutex _playerMutex;

	TlsMemoryPool<Player> _playerPool;
	TlsMemoryPool<Control> _controlPool;
};

