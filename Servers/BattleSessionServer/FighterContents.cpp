#include "FighterContents.h"
#include "FighterServer.h"
#include "CPacket.h"
#include"FighterSubStub.h"
#include "FighterProxy.h"
#include "ProxyTraceMemory.h"


MatchContents::MatchContents():Contents(MATCH,-1)
{
	StubForMatch* stub = new StubForMatch;
	AttachStub(stub);
}

MatchContents::~MatchContents()
{
	IStub* stub = DetachStub();
	delete stub;
}

void MatchContents::DisconnectAllPlayer()
{
	for (auto& iter : _playerMap)
	{
		Player* player = iter.second;
		_mServer->Disconnect(player->_sessionID);
	}
	if (_playerMap.empty())
	{
		if (CheckStopRequested())
		{
			FighterServer* server = static_cast<FighterServer*>(_mServer);
			Control* control = server->_controlPool.Alloc();
			control->_type = CONTROLTYPE::MATCHDEREGISTER;
			server->_ctrlQ.Enqueue(control);
			server->_ctrlCv.notify_one();
		}
	}
}



void MatchContents::OnEnter(__int64 sessionID, void* extra)
{
	if (_stopRequested.load()==true)
	{
		_mServer->Disconnect(sessionID);
		return;
	}

	FighterServer* server = static_cast<FighterServer*>(_mServer);
	ReadLock lock(server->_playerMutex);
	Player* player;
	std::unordered_map<SessionID, Player*>::iterator it = server->_playerMap.find(sessionID);
	if(it!=server->_playerMap.end())
	{
		player = it->second;

		_playerMap.insert(std::make_pair(sessionID, player));

		if (_playerMap.size() >= 6)
		{
			Control* control = server->_controlPool.Alloc();
			control->_type = CONTROLTYPE::FIGHTALLOC;
			std::unordered_map<SessionID, Player*>::iterator mit;
			for (int i = 0; i < 3; i++)
			{
				mit = _playerMap.begin();
				Player* player = mit->second;
				player->_team = Team::RED;
				control->_group._red[i] = player->_sessionID;
				_mServer->SetContentsNum(player->_sessionID, CONMOV);
				_playerMap.erase(mit);
			}
			for (int i = 0; i < 3; i++)
			{
				mit = _playerMap.begin();
				Player* player = mit->second;
				player->_team = Team::BLUE;
				control->_group._blue[i] = player->_sessionID;
				_mServer->SetContentsNum(player->_sessionID, CONMOV);
				_playerMap.erase(mit);
			}
			server->_ctrlQ.Enqueue(control);
			server->_ctrlCv.notify_one();
		}
	}

}

void MatchContents::OnLeave(__int64 sessionID, void* extra)
{
	_playerMap.erase(sessionID);
	if(CheckStopRequested())
	{
		FighterServer* server = static_cast<FighterServer*>(_mServer);
		Control* control = server->_controlPool.Alloc();
		control->_type = CONTROLTYPE::MATCHDEREGISTER;
		server->_ctrlQ.Enqueue(control);
		server->_ctrlCv.notify_one();
	}
}

void MatchContents::OnUpdate()
{
}

void MatchContents::OnShutDown()
{
	_stopRequested.store(true);
	DisconnectAllPlayer();
}

bool MatchContents::CheckStopRequested()
{
	if (_playerMap.empty() && _stopRequested.load() == true)
	{
		return true;
	}
	return false;
}

FightContents::FightContents():Contents(0,20)
{
	StubForFight* stub=new StubForFight;
	AttachStub(stub);
	FightProxy* proxy = new FightProxy;
	AttachProxy(proxy);
	_oldTick = timeGetTime();

}

FightContents::~FightContents()
{
	ProxyTraceBuffer::Write(
		ProxyTraceTag::Dtor,
		this,
		_proxy,
		_proxy ? _proxy->_server : nullptr,
		GetContentsNum(),
		0,
		_matched,
		static_cast<__int32>(_playerMap.size()),
		_redCount,
		_blueCount,
		_end);

	IStub* stub = DetachStub();
	delete stub;
	IProxy* proxy = DetachProxy();
	delete proxy;
}

void FightContents::OnEnter(__int64 sessionID, void* extra)
{
	FightProxy* proxy = static_cast<FightProxy*>(_proxy);

	ProxyTraceBuffer::Write(
		ProxyTraceTag::Enter,
		this,
		proxy,
		proxy ? proxy->_server : nullptr,
		GetContentsNum(),
		sessionID,
		_matched,
		static_cast<__int32>(_playerMap.size()),
		_redCount,
		_blueCount,
		_end);

	FighterServer* server = static_cast<FighterServer*>(_mServer);
	ReadLock lock(server->_playerMutex);
	Player* player;
	std::unordered_map<SessionID, Player*>::iterator it = server->_playerMap.find(sessionID);
	if (it != server->_playerMap.end())
	{
		player = it->second;
		bool moved=_mServer->SetContentsNum(player->_sessionID, GetContentsNum());
		if (moved)
		{
			player->_contents = GetContentsNum();
			if (player->_team == Team::RED)
			{
				_redCount++;
			}
			else
			{
				_blueCount++;
			}
			_playerMap.insert(std::make_pair(sessionID, player));
		}
		
	}

	++_matched;
	if (_matched >= 6)
	{
		if (CheckGameEnd() && _end)
		{
			_end = 0;
			std::unordered_map<SessionID, Player*>::iterator cit = _playerMap.begin();
			for (; cit != _playerMap.end(); cit++)
			{
				Player* tgt = cit->second;
				Disconnect(tgt->_sessionID);
			}

			BattleResult result;
			result._matchID = _matchID;
			for (int i = 0; i < 3; i++)
			{
				result._red[i] = _red[i];
				result._blue[i] = _blue[i];
			}
			result._winnerTeam = _winLoss;

			FighterServer* server = (FighterServer*)_mServer;
			server->RequestSaveBattleResult(result);

			Control* control = server->_controlPool.Alloc();
			control->_type = CONTROLTYPE::FIGHTFREE;
			control->_contents = this;

			server->_ctrlQ.Enqueue(control);
			server->_ctrlCv.notify_one();
			return;
		}
		int redcnt = 0;
		int bluecnt = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = _playerMap.begin();
		int id = 0;
		for( ; cit != _playerMap.end(); cit++)
		{
			Player* tgt = cit->second;
			tgt->_id = id++;
			tgt->_hp = 100;
			tgt->_move = -1;
			if (tgt->_team == Team::RED)		//red
			{
				_red[redcnt] = tgt->_sessionID;
				tgt->_x = REDX;
				tgt->_y = dfRANGE_MOVE_TOP+40 + TEAMY * redcnt;
				tgt->_direction = dfPACKET_MOVE_DIR_RR;
				redcnt++;
			}
			else	//blue
			{
				_blue[bluecnt] = tgt->_sessionID;
				tgt->_x = BLUEX;
				tgt->_y = dfRANGE_MOVE_TOP+40 + TEAMY * bluecnt;
				tgt->_direction = dfPACKET_MOVE_DIR_LL;
				bluecnt++;
			}
			__int64 sessionA[1] = { tgt->_sessionID };
			int count = 1;
			if (proxy == nullptr || proxy->_server == nullptr)
			{
				ProxyTraceBuffer::Write(
					ProxyTraceTag::EnterSkipNull,
					this,
					proxy,
					proxy ? proxy->_server : nullptr,
					GetContentsNum(),
					sessionID,
					_matched,
					static_cast<__int32>(_playerMap.size()),
					_redCount,
					_blueCount,
					_end);
			}
			((FightProxy*)_proxy)->ProxySCCreateMe(sessionA, 1, tgt->_id, tgt->_direction, tgt->_x, tgt->_y, tgt->_hp);
			ProxyTraceBuffer::Write(
				ProxyTraceTag::EnterAfterCreateMe,
				this,
				proxy,
				proxy ? proxy->_server : nullptr,
				GetContentsNum(),
				_debugGeneration,
				_matched,
				static_cast<__int32>(_playerMap.size()),
				_redCount,
				_blueCount,
				_end);
		}
		std::unordered_map<SessionID, Player*>::iterator oit = _playerMap.begin();
		for (; oit != _playerMap.end(); oit++)
		{
			Player* tgt = oit->second;
			std::unordered_map<SessionID, Player*>::iterator iit = _playerMap.begin();
			__int64 sessionA[6];
			int count = 0;
			for (; iit != _playerMap.end(); iit++)
			{
				Player* other = iit->second;
				if (other->_sessionID != tgt->_sessionID)
				{
					sessionA[count]=other->_sessionID;
					count++;
				}
			}
			if (proxy == nullptr || proxy->_server == nullptr)
			{
				ProxyTraceBuffer::Write(
					ProxyTraceTag::EnterSkipNull,
					this,
					proxy,
					proxy ? proxy->_server : nullptr,
					GetContentsNum(),
					sessionID,
					_matched,
					static_cast<__int32>(_playerMap.size()),
					_redCount,
					_blueCount,
					_end);
			}
			((FightProxy*)_proxy)->ProxySCCreateOther(sessionA, count, tgt->_id, tgt->_direction, tgt->_x, tgt->_y, tgt->_hp);
			ProxyTraceBuffer::Write(
				ProxyTraceTag::EnterAfterCreateOther,
				this,
				proxy,
				proxy ? proxy->_server : nullptr,
				GetContentsNum(),
				_debugGeneration,
				_matched,
				static_cast<__int32>(_playerMap.size()),
				_redCount,
				_blueCount,
				_end);
		}

	}
	ProxyTraceBuffer::Write(
		ProxyTraceTag::EnterEnd,
		this,
		proxy,
		proxy ? proxy->_server : nullptr,
		GetContentsNum(),
		_debugGeneration,
		_matched,
		static_cast<__int32>(_playerMap.size()),
		_redCount,
		_blueCount,
		_end);
}

void FightContents::OnLeave(__int64 sessionID, void* extra)
{
	FightProxy* proxy = static_cast<FightProxy*>(_proxy);

	ProxyTraceBuffer::Write(
		ProxyTraceTag::Leave,
		this,
		proxy,
		proxy ? proxy->_server : nullptr,
		GetContentsNum(),
		sessionID,
		_matched,
		static_cast<__int32>(_playerMap.size()),
		_redCount,
		_blueCount,
		_end);

	std::unordered_map<SessionID, Player*>::iterator it = _playerMap.find(sessionID);
	if (it == _playerMap.end())
	{
		return;
	}
	Player* player = it->second;

	if(player->_team==Team::RED)
	{
		_redCount--;
	}
	else
	{
		_blueCount--;
	}

	__int64 sessionA[6];
	int count = 0;
	std::unordered_map<SessionID, Player*>::iterator sit = _playerMap.begin();
	for (; sit != _playerMap.end(); sit++)
	{
		Player* tgt;
		tgt = sit->second;
		if (player->_sessionID != tgt->_sessionID)
		{
			sessionA[count++] = tgt->_sessionID;
		}
	}
	if (proxy == nullptr || proxy->_server == nullptr)
	{
		ProxyTraceBuffer::Write(
			ProxyTraceTag::LeaveSkipNull,
			this,
			proxy,
			proxy ? proxy->_server : nullptr,
			GetContentsNum(),
			sessionID,
			_matched,
			static_cast<__int32>(_playerMap.size()),
			_redCount,
			_blueCount,
			_end);
	}
	((FightProxy*)_proxy)->ProxySCDelete(sessionA, count, player->_id);

	_playerMap.erase(it);

	if (CheckGameEnd()&&_end)
	{
		_end = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = _playerMap.begin();
		for(;cit!= _playerMap.end(); cit++)
		{
			Player* tgt = cit->second;
			Disconnect(tgt->_sessionID);
		}

		BattleResult result;
		result._matchID = _matchID;
		for (int i = 0; i < 3; i++) 
		{
			result._red[i] = _red[i];
			result._blue[i] = _blue[i];
		}
		result._winnerTeam = _winLoss;

		FighterServer* server = (FighterServer*)_mServer;
		server->RequestSaveBattleResult(result);

		Control* control = server->_controlPool.Alloc();
		control->_type = CONTROLTYPE::FIGHTFREE;
		control->_contents = this;

		server->_ctrlQ.Enqueue(control);
		server->_ctrlCv.notify_one();
		
	}

}

void FightContents::OnUpdate()
{
	DWORD deltatime = timeGetTime() - _oldTick;
	DWORD frame = deltatime / 20;
	DWORD remain = deltatime % 20;
	_oldTick += (deltatime - remain);

	if (_proxy != nullptr && _proxy->_server == nullptr)
	{
		ProxyTraceBuffer::Write(
			ProxyTraceTag::UpdateNull,
			this,
			_proxy,
			_proxy ? _proxy->_server : nullptr,
			GetContentsNum(),
			0,
			_matched,
			static_cast<__int32>(_playerMap.size()),
			_redCount,
			_blueCount,
			_end);
		__debugbreak();
	}
	//frameCount++;
	//움직임 로직처리
	std::unordered_map<SessionID,Player*>::iterator moveit = _playerMap.begin();
	for (; moveit != _playerMap.end(); moveit++)
	{
		Player* player = moveit->second;
		if (player->_move != -1)
		{
			switch (player->_move)
			{
			case  dfPACKET_MOVE_DIR_LL:
			{
				if (player->_x > dfRANGE_MOVE_LEFT + 3)
				{
					player->_x -= 3 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_LU:
			{
				//왼쪽에 닿거나 위에 닿았을때
				if (player->_x <= dfRANGE_MOVE_LEFT + 3 || player->_y <= dfRANGE_MOVE_TOP)
				{
					//이동하면 안됨
				}
				else
				{
					player->_x -= 3 * frame;
					player->_y -= 2 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_UU:
			{
				if (player->_y > dfRANGE_MOVE_TOP)
				{
					player->_y -= 2 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_RU:
			{
				//오른쪽에 닿거나 위에 닿았을때
				if (player->_x >= dfRANGE_MOVE_RIGHT - 3 || player->_y <= dfRANGE_MOVE_TOP)
				{
					//이동하면 안됨
				}
				else
				{
					player->_x += 3 * frame;
					player->_y -= 2 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_RR:
			{
				if (player->_x < dfRANGE_MOVE_RIGHT - 3)
				{
					player->_x += 3 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_RD:
			{
				//오른쪽에 닿거나 아래에 닿았을때
				if (player->_x >= dfRANGE_MOVE_RIGHT - 3 || player->_y >= dfRANGE_MOVE_BOTTOM)
				{
					//이동하면 안됨
				}
				else
				{
					player->_x += 3 * frame;
					player->_y += 2 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_DD:
			{
				if (player->_y < dfRANGE_MOVE_BOTTOM)
				{
					player->_y += 2 * frame;
				}
				break;
			}
			case dfPACKET_MOVE_DIR_LD:
			{
				//왼쪽에 닿거나 아래에 닿았을때
				if (player->_x <= dfRANGE_MOVE_LEFT + 3 || player->_y >= dfRANGE_MOVE_BOTTOM)
				{
					//이동하면 안됨
				}
				else
				{
					player->_x -= 3 * frame;
					player->_y += 2 * frame;
				}
				break;
			}
			}
		}
	}
}

void FightContents::OnShutDown()
{}

void FightContents::Init(__int64 matchID)
{
	_matchID = matchID;
	ProxyTraceBuffer::Write(
		ProxyTraceTag::Init,
		this,
		_proxy,
		_proxy ? _proxy->_server : nullptr,
		GetContentsNum(),
		_debugGeneration,
		_matched,
		static_cast<__int32>(_playerMap.size()),
		_redCount,
		_blueCount,
		_end);
}

void FightContents::Clear()
{
	_playerMap.clear();
	_matchID = 0;
	_matched = 0;
	_redCount = 0;
	_blueCount = 0;
	_oldTick=0;

	_end = 1;
}
bool FightContents::CheckGameEnd()
{
	if (_redCount == 0)
	{
		_winLoss = 1;
		return true;
	}
	if(_blueCount == 0)
	{
		_winLoss = 2;
		return true;
	}
	return false;
}
