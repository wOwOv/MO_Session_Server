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
	MatchContents* match = new MatchContents;
	RegisterContents(MATCH, match);
	SetDefaultContents(MATCH);
}

FighterServer::~FighterServer()
{
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

__int64 FighterServer::CreateMatchID()
{
	return _matchIDGenerator.Create();
}


std::shared_mutex& FighterServer::GetPlayerLock()
{
	return _playerMutex;
}

unsigned __stdcall FighterServer::CtrlThread(LPVOID arg)		//FightContents는 무조건 애가 생성하고 삭제함
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
				break;
			}
			if (control->_type == 1)				//할당
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
			}
			if (control->_type == 2)				//삭제
			{
				//FightContents반환하고 유저들도 다시 옮겨야함
				server->DeregisterContents(((FightContents*)control->_contents)->GetContentsNum());
				server->_fightPool.Free((FightContents*)control->_contents);
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
	while (1)
	{
		DBRequest request;
		{
			std::unique_lock<std::mutex> lock(server->_dbMtx);
			server->_dbCv.wait(lock, [server]()
				{
					return !server->_dbQ.empty() || !server->_dbThreadRun;
				});
			if (!server->_dbThreadRun && server->_dbQ.empty())
			{
				break;
			}

			request = server->_dbQ.front();
			server->_dbQ.pop();
		}
		switch (request._type)
		{
		case DBRequestType::SaveBattleResult:
		{
			battleDB.SaveBattleResult(request._battleResult);
			break;
		}
		default:
		{
			break;
		}
		}

	}
}

void FighterServer::PushDBRequest(const DBRequest& request)
{
	{
		std::lock_guard<std::mutex> lock(_dbMtx);
		_dbQ.push(request);
	}

	_dbCv.notify_one();
}
