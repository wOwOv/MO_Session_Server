#include "FighterServer.h"
#include "DBConnector.h"
#include "BattleDB.h"
#include <thread>
#include <mutex>
#include "BattleMonitorMP.h"
#include "Profiler.h"



FighterServer::FighterServer() :ContentsServer(ServerType::LANSERVER),_matchIDGenerator(0),_fightPool(0,true,false)
{
	_ctrlThreadRun.store(true);
	_CtrlThread = std::thread(CtrlThread, this);
	_matchContents = std::make_shared<MatchContents>();
	RegisterContents(MATCH, _matchContents);
	SetDefaultContents(MATCH);
	_pdProducer = std::make_unique<PcPDProducer>();
	_monitorClient = std::make_unique<MonitorClient>(217);
	_monitorClient.get()->Start("MonitorClient_Config.txt");

}

FighterServer::~FighterServer()
{
}

bool FighterServer::FighterServerStart(const char* txtname, char code, char key)
{
	Start(txtname, code, key);
	if (!StartDBWorkers(txtname))
	{
		return false;
	}
	_state = SERVER_RUNNING;
	LOG(L"FighterServer", LVSYSTEM, L"FighterServer Started");

	return true;
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
	//4.FrameScheduler Thread 중지
	StopFrameSchedulerThread();
	printf("FrameScheduler Thread Stopped\n");
	//5. Worker Thread 중지
	StopWorkerThread();
	printf("Worker Thread Stopped\n");
	//6. DB Thread 중지
	StopDBWorkers();
	printf("DB Thread Stopped\n");
	StopMonitorThread();
	printf("Monitor Thread Stopped\n");
	_monitorClient.get()->Stop();
	printf("MonitorClient Stopped\n");
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

}

void FighterServer::OnUnusual(__int64 sessionID, const SOCKADDR_IN& clientaddr)
{
	WriteLock lock(_banMutex);
	_banSet.insert(clientaddr);
}

void FighterServer::OnSecond()
{
	int data;
	int timestamp;
	time_t temptime;

	data = 1;
	time(&temptime);
	timestamp = (int)temptime;
	CPacket runmsg;
	MPGameRun(&runmsg, data, timestamp);

	data = static_cast<int>(_pdProducer.get()->ProcessTotal());
	CPacket cpumsg;
	MPGameCpu(&cpumsg, data, timestamp);

	data = static_cast<int>(_pdProducer.get()->GetUserM());
	data /= 1024 * 1024;
	CPacket memmsg;
	MPGameMem(&memmsg, data, timestamp);

	data = GetSessionCount();
	CPacket sesmsg;
	MPGameSes(&sesmsg, data, timestamp);

	data = _matchContents.get()->GetPlayerCount();
	CPacket matmsg;
	MPGameAuthP(&matmsg, data, timestamp);

	data = GetSessionCount() - _matchContents.get()->GetPlayerCount();
	CPacket figmsg;
	MPGameGameP(&figmsg, data, timestamp);

	data = GetAcceptTPS();
	CPacket acpmsg;
	MPGameAcp(&acpmsg, data, timestamp);

	data = GetRecvMessageTPS();
	CPacket recvmsg;
	MPGameRcv(&recvmsg, data, timestamp);

	data = GetSendMessageTPS();
	CPacket sendmsg;
	MPGameSnd(&sendmsg, data, timestamp);

	data = _dbSaveCount.exchange(0);
	CPacket dbtpsmsg;
	MPGameDBTPS(&dbtpsmsg, data, timestamp);

	data = static_cast<int>(_dbQ.size());
	CPacket dbqmsg;
	MPGameDBMsg(&dbqmsg, data, timestamp);

	data = _fightAllocCount.exchange(0);
	CPacket fightallocmsg;
	MPGameAlloc(&fightallocmsg, data, timestamp);

	data = _fightFreeCount.exchange(0);
	CPacket fightfreemsg;
	MPGameFree(&fightfreemsg, data, timestamp);

	data = _ctrlQ.GetUsedSize();
	CPacket ctrlqmsg;
	MPGameCtrlQ(&ctrlqmsg, data, timestamp);

	data = GetSBufferUsingCount();
	CPacket packetmsg;
	MPGamePacket(&packetmsg, data, timestamp);

	data = _fightPool.GetUseCount();
	CPacket fightmsg;
	MPGameFightUsing(&fightmsg, data, timestamp);

	data = GetFpsAvg();
	CPacket fpsavgmsg;
	MPGameFightFPSAvg(&fpsavgmsg, data, timestamp);

	data = GetFpsMin();
	CPacket fpsminmsg;
	MPGameFightFPSMin(&fpsminmsg, data, timestamp);

	data = GetFpsMax();
	CPacket fpsmaxmsg;
	MPGameFightFPSMax(&fpsmaxmsg, data, timestamp);

	data = _dbSaveSuccessTotal.load();
	CPacket successmsg;
	MPGameDBSuccess(&successmsg, data, timestamp);

	data = _dbSaveFailureTotal.load();
	CPacket failmsg;
	MPGameDBFailure(&failmsg, data, timestamp);

	data = _dbDuplicateKeyTotal.load();
	CPacket dupmsg;
	MPGameDBDuplicateKey(&dupmsg, data, timestamp);

	data = _dbDeadlockTotal.load();
	CPacket dlockmsg;
	MPGameDBDeadlock(&dlockmsg, data, timestamp);

	data = _dbLockTimeoutTotal.load();
	CPacket timeoutmsg;
	MPGameDBLockTimeout(&timeoutmsg, data, timestamp);

	data = _dbConnectionLostTotal.load();
	CPacket clostmsg;
	MPGameDBConnectionLost(&clostmsg, data, timestamp);

	data = _dbQueryFormatErrorTotal.load();
	CPacket qerrormsg;
	MPGameDBQueryFormatError(&qerrormsg, data, timestamp);
	
	data = _dbUnknownErrorTotal.load();
	CPacket unknownmsg;
	MPGameDBUnknownError(&unknownmsg, data, timestamp);


	if (_monitorClient.get() != nullptr)
	{
		_monitorClient.get()->SendPacket(runmsg);
		_monitorClient.get()->SendPacket(cpumsg);
		_monitorClient.get()->SendPacket(memmsg);
		_monitorClient.get()->SendPacket(sesmsg);
		_monitorClient.get()->SendPacket(matmsg);
		_monitorClient.get()->SendPacket(figmsg);
		_monitorClient.get()->SendPacket(acpmsg);
		_monitorClient.get()->SendPacket(recvmsg);
		_monitorClient.get()->SendPacket(sendmsg);
		_monitorClient.get()->SendPacket(dbtpsmsg);
		_monitorClient.get()->SendPacket(dbqmsg);
		_monitorClient.get()->SendPacket(fightallocmsg);
		_monitorClient.get()->SendPacket(fightfreemsg);
		_monitorClient.get()->SendPacket(ctrlqmsg);
		_monitorClient.get()->SendPacket(packetmsg);
		_monitorClient.get()->SendPacket(fightmsg);
		_monitorClient.get()->SendPacket(fpsavgmsg);
		_monitorClient.get()->SendPacket(fpsminmsg);
		_monitorClient.get()->SendPacket(fpsmaxmsg);
		_monitorClient.get()->SendPacket(successmsg);
		_monitorClient.get()->SendPacket(failmsg);
		_monitorClient.get()->SendPacket(dupmsg);
		_monitorClient.get()->SendPacket(dlockmsg);
		_monitorClient.get()->SendPacket(timeoutmsg);
		_monitorClient.get()->SendPacket(clostmsg);
		_monitorClient.get()->SendPacket(qerrormsg);
		_monitorClient.get()->SendPacket(unknownmsg);
	}

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
	std::lock_guard<std::shared_mutex> lock(_playerMutex);
	return static_cast<int>(_playerMap.size());
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

	printf("FightResourceCapacity: %d\nFightResourceUsing: %d\nPlayerPoolCapacity: %d\nPlayerPoolUsing: %d\nControlPoolCapacity: %d\nControlPoolUsing: %d\nPlayer: %d\n",
		GetFightPoolCapacity(),GetFightPoolUsingCount(),GetPlayerPoolCapacity(),GetPlayerPoolUsingCount(),GetControlPoolCapacity(),GetControlPoolUsingCount(),GetPlayerCount());
	printf("DBSuccessTotal: %llu      DBFailureTotal: %llu\nDBDuplicateKey: %llu      DBDeadlock: %llu\nDBLockTimeout: %llu      DBConnectionLost: %llu\nDBQueryFormatError: %llu      DBUnknownError: %llu\n\n",
		_dbSaveSuccessTotal.load(), _dbSaveFailureTotal.load(), _dbDuplicateKeyTotal.load(), _dbDeadlockTotal.load(),
		_dbLockTimeoutTotal.load(), _dbConnectionLostTotal.load(), _dbQueryFormatErrorTotal.load(), _dbUnknownErrorTotal.load());
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

bool FighterServer::StartDBWorkers(const char* txtname)
{
	Parser parser;
	parser.LoadFile(txtname);

	int workerCount = 1;
	parser.GetValue("FighterServer", "DBWorkerCount", &workerCount);
	if (workerCount > 0 && workerCount < 5)
	{
		_dbWorkerCount = workerCount;
	}
	else
	{
		LOG(L"Database", LVERROR, L"Wrong Database WorkerThreads Number : %d", workerCount);
		return false;
	}

	_dbThreadRun.store(true);
	_dbWorkers.reserve(_dbWorkerCount);

	for (int index = 0; index < _dbWorkerCount; ++index)
	{
		_dbWorkers.emplace_back(DBThread, this);
	}
	return true;
}

void FighterServer::StopDBWorkers()
{
	{
		std::lock_guard<std::mutex> lock(_dbMtx);
		_dbThreadRun.store(false);
	}

	_dbCv.notify_all();

	for (std::thread& worker : _dbWorkers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}

	_dbWorkers.clear();
	_state.store(DB_STOPPED);

	LOG(L"FighterServer", LVSYSTEM, L"All DB workers stopped.");
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
	__int32 cnum = 1;
	FighterServer* server = static_cast<FighterServer*>(arg);

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
				if (server->ShouldStopControlThread())
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
				server->HandleFightAlloc(*control, cnum);
				server->_fightAllocCount.fetch_add(1);
				break;

			case CONTROLTYPE::FIGHTFREE:
				server->HandleFightFree(*control);
				server->_fightFreeCount.fetch_add(1);
				break;

			case CONTROLTYPE::MATCHDEREGISTER:
				server->HandleMatchDeregister();
				break;

			default:
				break;
			}

			server->_controlPool.Free(control);
		}
	}
}
unsigned __stdcall FighterServer::DBThread(LPVOID arg)
{
	DBConnector db("DBInfo.txt");
	if (!db.Connect())
	{
		LOG(L"Database", LVERROR, L"DB worker failed to connect. thread_id=%lu", GetCurrentThreadId());
		return 0;
	}
	BattleDB battleDB(db);
	FighterServer* server = (FighterServer*)arg;
	while (true)
	{
		DBRequest request;

		if (!server->WaitAndPopDBRequest(request))
		{
			if (server->_dbThreadRun.load() == false && server->_state.load() == CONTROL_STOPPED)
			{
				LOG(L"FighterServer", LVSYSTEM, L"DB Thread Stopped");
				return 0;
			}
			break;
		}

		switch (request._type)
		{
		case DBRequestType::SaveBattleResult:
		{
			const DBSaveResult saveResult =	battleDB.SaveBattleResult(request._battleResult);
			server->RecordDBSaveCounters(saveResult);
			if (!saveResult.succeeded)
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

void FighterServer::HandleFightAlloc(Control& control, __int32& nextContentsNum)
{
	// 변경
	std::shared_ptr<FightContents> contents(
		_fightPool.Alloc(),
		[this](FightContents* fight)
		{
			_fightPool.Free(fight);
		});

	contents->Clear();
	contents->Init(nextContentsNum, CreateMatchID(), control._group);

	RegisterContents(nextContentsNum, contents);

	for (int i = 0; i < 3; ++i)
	{
		InsertToContents(control._group._red[i], nextContentsNum);
	}

	for (int i = 0; i < 3; ++i)
	{
		InsertToContents(control._group._blue[i], nextContentsNum);
	}

	AdvanceContentsNum(nextContentsNum);
}

void FighterServer::HandleFightFree(Control& control)
{
	FightContents* fight = static_cast<FightContents*>(control._contents);
	DeregisterContents(fight->GetContentsNum());
}

void FighterServer::HandleMatchDeregister()
{
	DeregisterContents(MATCH);
	_state.store(MATCH_DEREGISTERED);
	LOG(L"FighterServer", LVSYSTEM, L"Match Contents Deregistered");
}

bool FighterServer::ShouldStopControlThread()
{
	return _fightPool.GetUseCount() == 0
		&& _ctrlThreadRun.load() == false
		&& _state.load() == MATCH_DEREGISTERED;
}

void FighterServer::AdvanceContentsNum(__int32& nextContentsNum) const
{
	++nextContentsNum;
	nextContentsNum %= ROOM;

	if (nextContentsNum == 0)
	{
		++nextContentsNum;
	}
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

void FighterServer::RecordDBSaveCounters(const DBSaveResult& saveResult)
{
	_dbSaveCount.fetch_add(1);

	if (saveResult.succeeded)
	{
		_dbSaveSuccessTotal.fetch_add(1);
		return;
	}

	_dbSaveFailureTotal.fetch_add(1);

	switch (saveResult.error.category)
	{
	case DBErrorCategory::DuplicateKey:
		_dbDuplicateKeyTotal.fetch_add(1);
		break;

	case DBErrorCategory::Deadlock:
		_dbDeadlockTotal.fetch_add(1);
		break;

	case DBErrorCategory::LockTimeout:
		_dbLockTimeoutTotal.fetch_add(1);
		break;

	case DBErrorCategory::ConnectionLost:
		_dbConnectionLostTotal.fetch_add(1);
		break;

	case DBErrorCategory::QueryFormatError:
		_dbQueryFormatErrorTotal.fetch_add(1);
		break;

	case DBErrorCategory::None:
	case DBErrorCategory::Unknown:
	default:
		_dbUnknownErrorTotal.fetch_add(1);
		break;
	}
}
