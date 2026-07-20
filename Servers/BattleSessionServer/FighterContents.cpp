#include "FighterContents.h"
#include "FighterServer.h"
#include "CPacket.h"
#include"FighterSubStub.h"
#include "FighterProxy.h"


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

			AssignTeamAndMovePlayers(control->_group._red, Team::RED);
			AssignTeamAndMovePlayers(control->_group._blue, Team::BLUE);

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

int MatchContents::GetPlayerCount()
{
	return _playerMap.size();
}

bool MatchContents::CheckStopRequested()
{
	if (_playerMap.empty() && _stopRequested.load() == true)
	{
		return true;
	}
	return false;
}

void MatchContents::AssignTeamAndMovePlayers(SessionID(&teamSlots)[3], Team team)
{
	for (int i = 0; i < 3; ++i)
	{
		auto it = _playerMap.begin();
		Player* player = it->second;

		player->_team = team;
		teamSlots[i] = player->_sessionID;
		_mServer->TryMoveSessionToContents(player->_sessionID, CONMOV);

		_playerMap.erase(it);
	}
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
	IStub* stub = DetachStub();
	delete stub;
	IProxy* proxy = DetachProxy();
	delete proxy;
}

void FightContents::OnEnter(__int64 sessionID, void* extra)
{
	FighterServer* server = static_cast<FighterServer*>(_mServer);
	ReadLock lock(server->_playerMutex);
	Player* player;
	std::unordered_map<SessionID, Player*>::iterator it = server->_playerMap.find(sessionID);
	if (it != server->_playerMap.end())
	{
		player = it->second;
		bool moved=_mServer->TryMoveSessionToContents(player->_sessionID, GetContentsNum());
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
			FinishFightAndRelease();
			return;
		}

		for (int i = 0; i < 3; ++i)
		{
			auto rit = _playerMap.find(_red[i]);
			if (rit != _playerMap.end())
			{
				Player* tgt = rit->second;
				tgt->_id = i;
				tgt->_hp = 100;
				tgt->_move = -1;
				tgt->_x = REDX;
				tgt->_y = dfRANGE_MOVE_TOP + 40 + TEAMY * i;
				tgt->_direction = dfPACKET_MOVE_DIR_RR;

				__int64 sessionA[1] = { tgt->_sessionID };
				((FightProxy*)_proxy)->ProxySCCreateMe(sessionA, 1, tgt->_id, tgt->_direction, tgt->_x, tgt->_y, tgt->_hp);
			}

			auto bit = _playerMap.find(_blue[i]);
			if (bit != _playerMap.end())
			{
				Player* tgt = bit->second;
				tgt->_id = i+3;
				tgt->_hp = 100;
				tgt->_move = -1;
				tgt->_x = BLUEX;
				tgt->_y = dfRANGE_MOVE_TOP + 40 + TEAMY * i;
				tgt->_direction = dfPACKET_MOVE_DIR_LL;

				__int64 sessionA[1] = { tgt->_sessionID };
				((FightProxy*)_proxy)->ProxySCCreateMe(sessionA, 1, tgt->_id, tgt->_direction, tgt->_x, tgt->_y, tgt->_hp);
			}
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
			((FightProxy*)_proxy)->ProxySCCreateOther(sessionA, count, tgt->_id, tgt->_direction, tgt->_x, tgt->_y, tgt->_hp);
		}

	}
}

void FightContents::OnLeave(__int64 sessionID, void* extra)
{
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
	((FightProxy*)_proxy)->ProxySCDelete(sessionA, count, player->_id);

	_playerMap.erase(it);

	if (CheckGameEnd()&&_end)
	{
		FinishFightAndRelease();
		return;
	}
}

void FightContents::OnUpdate()
{
	DWORD deltatime = timeGetTime() - _oldTick;
	DWORD frame = deltatime / 20;
	DWORD remain = deltatime % 20;
	_oldTick += (deltatime - remain);

	//框流烙 肺流贸府
	const int frameCount = static_cast<int>(frame);

	std::unordered_map<SessionID, Player*>::iterator moveit = _playerMap.begin();
	for (; moveit != _playerMap.end(); moveit++)
	{
		Player* player = moveit->second;
		if (player->_move != -1)
		{
			ApplyMovement(*player, frameCount);
		}
	}
}

void FightContents::OnShutDown()
{}

void FightContents::Init(__int32 contentsNum, __int64 matchID, const Group& group)
{
	SetContentsNum(contentsNum);
	_matchID = matchID;

	for (int i = 0; i < 3; ++i)
	{
		_red[i] = group._red[i];
		_blue[i] = group._blue[i];
	}
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
	for (int i = 0; i < 3; ++i)
	{
		_red[i] = 0;
		_blue[i] = 0;
	}
	_winLoss = 0;
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

void FightContents::ApplyMovement(Player& player, int frame)
{
	if (frame <= 0)
	{
		return;
	}

	const int currentX = static_cast<int>(player._x);
	const int currentY = static_cast<int>(player._y);

	int newX = currentX;
	int newY = currentY;

	switch (player._move)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		const int maxFrameX = (currentX - dfRANGE_MOVE_LEFT) / 3;
		const int moveFrame = (frame < maxFrameX) ? frame : maxFrameX;
		newX = currentX - 3 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_UU:
	{
		const int maxFrameY = (currentY - dfRANGE_MOVE_TOP) / 2;
		const int moveFrame = (frame < maxFrameY) ? frame : maxFrameY;
		newY = currentY - 2 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_RR:
	{
		const int maxFrameX = (dfRANGE_MOVE_RIGHT - currentX) / 3;
		const int moveFrame = (frame < maxFrameX) ? frame : maxFrameX;
		newX = currentX + 3 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_DD:
	{
		const int maxFrameY = (dfRANGE_MOVE_BOTTOM - currentY) / 2;
		const int moveFrame = (frame < maxFrameY) ? frame : maxFrameY;
		newY = currentY + 2 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_LU:
	{
		const int maxFrameX = (currentX - dfRANGE_MOVE_LEFT) / 3;
		const int maxFrameY = (currentY - dfRANGE_MOVE_TOP) / 2;
		int moveFrame = maxFrameX < maxFrameY ? maxFrameX : maxFrameY;
		moveFrame = frame < moveFrame ? frame : moveFrame;

		newX = currentX - 3 * moveFrame;
		newY = currentY - 2 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_RU:
	{
		const int maxFrameX = (dfRANGE_MOVE_RIGHT - currentX) / 3;
		const int maxFrameY = (currentY - dfRANGE_MOVE_TOP) / 2;
		int moveFrame = maxFrameX < maxFrameY ? maxFrameX : maxFrameY;
		moveFrame = frame < moveFrame ? frame : moveFrame;

		newX = currentX + 3 * moveFrame;
		newY = currentY - 2 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_RD:
	{
		const int maxFrameX = (dfRANGE_MOVE_RIGHT - currentX) / 3;
		const int maxFrameY = (dfRANGE_MOVE_BOTTOM - currentY) / 2;
		int moveFrame = maxFrameX < maxFrameY ? maxFrameX : maxFrameY;
		moveFrame = frame < moveFrame ? frame : moveFrame;

		newX = currentX + 3 * moveFrame;
		newY = currentY + 2 * moveFrame;
		break;
	}

	case dfPACKET_MOVE_DIR_LD:
	{
		const int maxFrameX = (currentX - dfRANGE_MOVE_LEFT) / 3;
		const int maxFrameY = (dfRANGE_MOVE_BOTTOM - currentY) / 2;
		int moveFrame = maxFrameX < maxFrameY ? maxFrameX : maxFrameY;
		moveFrame = frame < moveFrame ? frame : moveFrame;

		newX = currentX - 3 * moveFrame;
		newY = currentY + 2 * moveFrame;
		break;
	}

	default:
		return;
	}

	player._x = static_cast<std::uint16_t>(newX);
	player._y = static_cast<std::uint16_t>(newY);
}

void FightContents::FinishFightAndRelease()
{
	_end = 0;
	DisconnectRemainingPlayers();
	BattleResult result = BuildBattleResult();
	FighterServer* server = (FighterServer*)_mServer;
	server->RequestSaveBattleResult(result);
	EnqueueFightFree();
}

void FightContents::DisconnectRemainingPlayers()
{
	for (const auto& entry : _playerMap)
	{
		Disconnect(entry.second->_sessionID);
	}
}

BattleResult FightContents::BuildBattleResult() const
{
	BattleResult result;
	result._matchID = _matchID;
	for (int i = 0; i < 3; i++)
	{
		result._red[i] = _red[i];
		result._blue[i] = _blue[i];
	}
	result._winnerTeam = _winLoss;
	return result;
}

void FightContents::EnqueueFightFree()
{
	FighterServer* server = (FighterServer*)_mServer;

	Control* control = server->_controlPool.Alloc();
	control->_type = CONTROLTYPE::FIGHTFREE;
	control->_contents = this;

	server->_ctrlQ.Enqueue(control);
	server->_ctrlCv.notify_one();
}
