#include "FighterServer.h"
#include "FighterStructure.h"
#include "DBConnector.h"
#include "BattleDB.h"
#include <thread>


FighterServer::FighterServer() :ContentsServer(LANSERVER),_matchIDGenerator(0)
{
	_CtrlThread = std::thread(CtrlThread, this);
	_dbThreadRun = true;
	_DBThread = std::thread(DBThread, this);
	_matchContents = new MatchContents;
	RegisterContents(MATCH, _matchContents);
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


bool FighterServer::OnConnectionRequest(SOCKADDR_IN* clientaddr)
{
	if (_banSet.find(*clientaddr) != _banSet.end())
	{
		return false;
	}
	return true;
}

void FighterServer::OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID)
{
	Player* player = _playerPool.Alloc();
	player->_sessionID = sessionID;
	player->_clientAddr = *clientaddr;
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

void FighterServer::OnUnusual(__int64 sessionID, SOCKADDR_IN clientaddr)
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

void FighterServer::RequestDeregisterMatchContents()
{
	_matchContents->StopMatch();
}

void FighterServer::StopControlThread()
{
	{
		std::lock_guard<std::mutex> lock(_dbMtx);
		_dbThreadRun.store(false);
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
	server->_ctrlThreadRun.store(true);
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
				if (server->_ctrlThreadRun.load() == false)
				{
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
				break;
			}
			case CONTROLTYPE::FIGHTFREE:
			{
				FightContents* fight = (FightContents*)control->_contents;
				server->DeregisterContents(fight->GetContentsNum());
				server->_fightPool.Free(fight);
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

void FighterServer::PushDBRequest(const DBRequest& request)
{
	{
		std::lock_guard<std::mutex> lock(_dbMtx);
		_dbQ.push(request);
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
