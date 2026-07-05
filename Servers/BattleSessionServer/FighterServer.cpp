#include "FighterServer.h"
#include "DBConnector.h"
#include "BattleDB.h"
#include <thread>


FighterServer::FighterServer() :ContentsServer(ServerType::LANSERVER),_matchIDGenerator(0),_fightPool(0,true,false)
{
	_ctrlThreadRun.store(true);
	_CtrlThread = std::thread(CtrlThread, this);
	_dbThreadRun.store(true);
	_DBThread = std::thread(DBThread, this);
	_matchContents = std::make_unique<MatchContents>();
	RegisterContents(MATCH, _matchContents.get());
	SetDefaultContents(MATCH);
}

FighterServer::~FighterServer()
{
}

void FighterServer::FighterServerStart(const char* txtname, char code, char key)
{
	Start(txtname, code, key);
	_state = SERVER_RUNNING;
	LOG(L"FighterServer", LVSYSTEM, L"FighterServer Started");
}

void FighterServer::Stop()
{
	//1.라이브러리 측 Accept중지
	StopAcceptThread();
	_state.store(ACCEPT_STOPPED);
	printf("Accept Thread Stopped\n");
	//2. MatchContetns Deregister
	RequestStopMatchContents();
	while (_state < MATCH_DEREGISTERED)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	printf("MatchContents Deregistered\n");
	//3. Control Thread 중지
	StopControlThread();
	printf("Control Thread Stopped\n");
	//4. Worker Thread 중지
	StopWorkerThread();
	printf("Worker Thread Stopped\n");
	//5. DB Thread 중지
	StopDBThread();
	printf("DB Thread Stopped\n");
	StopMonitorThread();
	printf("Monitor Thread Stopped\n");
}


bool FighterServer::OnConnectionRequest(const SOCKADDR_IN& clientaddr)
{
	if (_banSet.find(clientaddr) != _banSet.end())
	{
		return false;
	}
	return true;
}

void FighterServer::OnAccept(const SOCKADDR_IN& clientaddr, __int64 sessionID)
{
	Player* player = _playerPool.Alloc();
	player->_sessionID = sessionID;
	player->_clientAddr = clientaddr;
	WriteLock lock(_playerMutex);
	_playerMap.insert(std::make_pair(sessionID, player));
}

void FighterServer::OnRelease(__int64 sessionID, __int32 contentsnum)
{
	WriteLock lock(_playerMutex);
	Player* player;
	std::unordered_map<SessionID, Player*>::iterator it = _playerMap.find(sessionID);
	if (it != _playerMap.end())
	{
		player = it->second;
		_playerMap.erase(sessionID);
		_playerPool.Free(player);
	}
	playerRelease.fetch_add(1);

}

void FighterServer::OnUnusual(__int64 sessionID, const SOCKADDR_IN& clientaddr)
{
	WriteLock lock(_banMutex);
	_banSet.insert(clientaddr);
}

void FighterServer::OnSecond()
{
}

int FighterServer::GetFightPoolCapacity()
{
	return _fightPool.GetCapacityCount();
}

int FighterServer::GetFightPoolUsingCount()
{
	return _fightPool.GetUseCount();
}

int FighterServer::GetControlQSize()
{
	return _ctrlQ.GetUsedSize();
}

int FighterServer::GetPlayerCount()
{
	return _playerMap.size();
}

int FighterServer::GetPlayerPoolCapacity()
{
	return _playerPool.GetCapacity();
}

int FighterServer::GetPlayerPoolUsingCount()
{
	return _playerPool.GetUsingCount();
}

int FighterServer::GetControlPoolCapacity()
{
	return _controlPool.GetCapacity();
}

int FighterServer::GetControlPoolUsingCount()
{
	return _controlPool.GetUsingCount();
}

void FighterServer::ShowServerInfo()
{
	ContentsServer::ShowServerInfo();

	switch (_state)
	{
	case SERVER_CREATED:
	{
		printf("Server State : SERVER_CREATED\n");
		break;
	}
	case SERVER_RUNNING:
	{
		printf("Server State : SERVER_RUNNING\n");
		break;
	}
	case ACCEPT_STOPPED:
	{
		printf("Server State : ACCEPT_STOPPED\n");
		break;
	}
	case MATCH_DEREGISTERED:
	{
		printf("Server State : MATCH_DEREGISTERED\n");
		break;
	}
	case CONTROL_STOPPED:
	{
		printf("Server State : CONTROL_STOPPED\n");
		break;
	}
	case DB_STOPPED:
	{
		printf("Server State : DB_STOPPED\n");
		break;
	}
	}

	printf("FightResourceCapacity: %d\nFightResourceUsing: %d\nPlayerPoolCapacity: %d\nPlayerPoolUsing: %d\nControlPoolCapacity: %d\nControlPoolUsing: %d\nPlayer: %d",
		GetFightPoolCapacity(),GetFightPoolUsingCount(),GetPlayerPoolCapacity(),GetPlayerPoolUsingCount(),GetControlPoolCapacity(),GetControlPoolUsingCount(),GetPlayerCount());
}

void FighterServer::OtherServerControl(int controlKey)
{
	if (controlKey == 'B' || controlKey == 'b')
	{
		_shutDown = true;
	}
}

bool FighterServer::IsShutDownRequested()
{
	return _shutDown;
}

void FighterServer::RequestStopMatchContents()
{
	PostQueueContentsShutDown(MATCH);
}

void FighterServer::StopControlThread()
{
	{
		std::lock_guard<std::mutex> lock(_ctrlMtx);
		_ctrlThreadRun.store(false);
	}

	_ctrlCv.notify_one();

	if (_CtrlThread.joinable())
	{
		_CtrlThread.join();
	}
	_state.store(CONTROL_STOPPED);
	LOG(L"FighterServer", LVSYSTEM, L"Control Thread Stopped");
}

void FighterServer::StopDBThread()
{
	{
		std::lock_guard<std::mutex> lock(_dbMtx);
		_dbThreadRun.store(false);
	}

	_dbCv.notify_one();

	if (_DBThread.joinable())
	{
		_DBThread.join();
	}
	_state.store(DB_STOPPED);
	LOG(L"FighterServer", LVSYSTEM, L"DB Thread Stopped");
}

__int64 FighterServer::CreateMatchID()
{
	return _matchIDGenerator.Create();
}

void FighterServer::RequestSaveBattleResult(const BattleResult& result)
{
	DBRequest request;
	request._type = DBRequestType::SaveBattleResult;
	request._battleResult = result;

	PushDBRequest(request);
}


std::shared_mutex& FighterServer::GetPlayerLock()
{
	return _playerMutex;
}

unsigned __stdcall FighterServer::CtrlThread(LPVOID arg)		//FightContents는 CtrlThread가 생성하고 삭제함
{
	__int32 cnum = 1;//1~1000000000까지 부여가능

	FighterServer* server = (FighterServer*)arg;
	while (1)
	{
		std::unique_lock<std::mutex> lock(server->_ctrlMtx);
		server->_ctrlCv.wait(lock);

		while (1)
		{
			Control* control;
			bool check = server->_ctrlQ.Dequeue(&control);
			if (check == false)
			{
				if (server->_fightPool.GetUseCount()==0&&server->_ctrlThreadRun.load() == false&&server->_state.load()==MATCH_DEREGISTERED)
				{
					server->_state.store(CONTROL_STOPPED);
					LOG(L"FighterServer", LVSYSTEM, L"Control Thread Stopped");
					return 0;
				}
				break;
			}
			switch (control->_type)
			{
			case CONTROLTYPE::FIGHTALLOC:
			{
				FightContents* contents = server->_fightPool.Alloc();
				contents->Clear();
				contents->Init(server->CreateMatchID());
				contents->SetContentsNum(cnum);

				server->RegisterContents(cnum, contents);
				for (int i = 0; i < 3; i++)
				{
					server->InsertToContents(control->_group._red[i], cnum);
				}
				for (int i = 0; i < 3; i++)
				{
					server->InsertToContents(control->_group._blue[i], cnum);
				}

				++cnum;
				cnum %= ROOM;
				if (cnum == 0)
				{
					++cnum;
				}
				server->fightAllocExecute.fetch_add(1);
				break;
			}
			case CONTROLTYPE::FIGHTFREE:
			{
				FightContents* fight = (FightContents*)control->_contents;
				server->DeregisterContents(fight->GetContentsNum());
				server->_fightPool.Free(fight);
				server->fightFreeExecute.fetch_add(1);
				break;
			}
			case CONTROLTYPE::MATCHDEREGISTER:
			{
				server->DeregisterContents(MATCH);
				server->_state.store(MATCH_DEREGISTERED);
				LOG(L"FighterServer", LVSYSTEM, L"Match Contents Deregistered");
				break;
			}
			default:
			{
				break;
			}
			}
			server->_controlPool.Free(control);
		}
	}
}
unsigned __stdcall FighterServer::DBThread(LPVOID arg)
{
	DBConnector db("DBInfo.txt");
	db.Connect();
	BattleDB battleDB(db);
	FighterServer* server = (FighterServer*)arg;
	while (true)
	{
		DBRequest request;

		if (!server->WaitAndPopDBRequest(request))
		{
			if (server->_dbThreadRun.load() == false && server->_state.load() == CONTROL_STOPPED)
			{
				server->_state.store(DB_STOPPED);
				LOG(L"FIghterServer", LVSYSTEM, L"DB Thread Stopped");
				return 0;
			}
			break;
		}

		switch (request._type)
		{
		case DBRequestType::SaveBattleResult:
		{
			if (!battleDB.SaveBattleResult(request._battleResult))
			{
				LOG(L"Database", LVSYSTEM,
					L"Battle result DB save failed. match_id=%lld",
					request._battleResult._matchID);
			}

			break;
		}

		default:
		{
			LOG(L"Database", LVSYSTEM,
				L"Unknown DB request type.");
			break;
		}
		}
	}
	return 1;
}

void FighterServer::PushDBRequest(DBRequest request)
{
	{
		std::lock_guard<std::mutex> lock(_dbMtx);
		_dbQ.push(std::move(request));
	}

	_dbCv.notify_one();
}

bool FighterServer::WaitAndPopDBRequest(DBRequest& outrequest)
{
	std::unique_lock<std::mutex> lock(_dbMtx);

	_dbCv.wait(lock, [this]()
		{
			return !_dbQ.empty() || !_dbThreadRun;
		});

	if (!_dbThreadRun && _dbQ.empty())
	{
		return false;
	}

	outrequest = _dbQ.front();
	_dbQ.pop();

	return true;
}
