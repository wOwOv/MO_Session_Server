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
private:
	enum ServerState : std::uint8_t
	{
		SERVER_CREATED = 0, 
		SERVER_RUNNING = 1,
		ACCEPT_STOPPED = 2,
		MATCH_DEREGISTERED = 3,
		CONTROL_STOPPED=4,
		DB_STOPPED = 5,
	};
public:
	FighterServer();
	~FighterServer();

	FighterServer(const FighterServer&) = delete;
	FighterServer& operator=(const FighterServer&) = delete;
	FighterServer(FighterServer&&) = delete;
	FighterServer& operator=(FighterServer&&) = delete;

	void FighterServerStart(const char* txtname, char code = 0, char key = 0);
	virtual void Stop() override;

	//virtual void ServerControl();
	virtual void ShowServerInfo() override;
	virtual void OtherServerControl(int controlKey) override;

	bool IsShutDownRequested();

	void RequestStopMatchContents();
	void StopControlThread();
	void StopDBThread();

	virtual bool OnConnectionRequest(const SOCKADDR_IN& clientaddr) override;
	virtual void OnAccept(const SOCKADDR_IN& clientaddr, __int64 sessionID) override;
	virtual void OnRelease(__int64 sessionID, __int32 contentsnum)override;
	virtual void OnUnusual(__int64 sessionID, const SOCKADDR_IN& clientaddr) override;
	virtual void OnSecond() override;

	int GetFightPoolCapacity();
	int GetFightPoolUsingCount();
	int GetControlQSize();
	int GetPlayerCount();
	int GetPlayerPoolCapacity();
	int GetPlayerPoolUsingCount();
	int GetControlPoolCapacity();
	int GetControlPoolUsingCount();

	__int64 CreateMatchID();

	void RequestSaveBattleResult(const BattleResult& result);

	std::shared_mutex& GetPlayerLock();

private:
	static unsigned __stdcall CtrlThread(LPVOID arg);
	static unsigned __stdcall DBThread(LPVOID arg);

private:
	void PushDBRequest(DBRequest request);
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
public:
	std::atomic<int> fightAllocRequest = 0;
	std::atomic<int> fightFreeRequest1 = 0;
	std::atomic<int> fightFreeRequest2 = 0;
	std::atomic<int> fightAllocExecute = 0;
	std::atomic<int> fightFreeExecute = 0;
	std::atomic<int> playerRelease = 0;
	std::atomic<int> matchRelease = 0;
	std::atomic<int> fightRelease = 0;
	std::atomic<int> MatchOnEnter = 0;
	std::atomic<int> FightOnEnter = 0;
	std::atomic<int> FightOnLeaveFindFail = 0;
	std::atomic<int> FightOnEnterFindSuccess = 0;
	std::atomic<int> FightOnEnterFindFail = 0;


private:

	bool _shutDown = false;
	std::atomic<ServerState> _state= ServerState::SERVER_CREATED;

	//MatchContents
	std::unique_ptr<MatchContents> _matchContents;

	//제어스레드
	std::thread _CtrlThread;
	LFQueue<Control*> _ctrlQ;
	std::condition_variable _ctrlCv;
	std::mutex _ctrlMtx;
	MemoryPool<FightContents> _fightPool;
	std::atomic<bool> _ctrlThreadRun;

	MatchIDGenerator _matchIDGenerator;

	//DB저장스레드
	std::thread _DBThread;
	std::queue<DBRequest> _dbQ;
	std::condition_variable _dbCv;
	std::mutex _dbMtx;
	std::atomic<bool> _dbThreadRun;


	std::unordered_set<SOCKADDR_IN, SockAddrInHash, SockAddrInEqual> _banSet;
	std::shared_mutex _banMutex;

	std::unordered_map<SessionID,Player*> _playerMap;// non-owning; Player objects are owned by _playerPool
	std::shared_mutex _playerMutex;

	TlsMemoryPool<Player> _playerPool;
	TlsMemoryPool<Control> _controlPool;
};

