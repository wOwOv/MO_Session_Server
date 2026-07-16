#include "FighterSubStub.h"
#include "FighterProxy.h"
#include "FighterDefine.h"
#include "FighterStructure.h"

void StubForMatch::ProcMatchDefault(__int64 sessionID, CPacket packet)
{
	_server->Disconnect(sessionID);
}

void StubForFight::ProcCSMoveStart(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = FindPlayer(sessionID);
	if (player == nullptr)
	{
		return;
	}

	if (!IsPositionSyncValid(*player, x, y))
	{
		_server->Disconnect(sessionID);
		return;
	}
	//player 움직임 정보 변경
	switch (dir)
	{
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		player->_direction = dfPACKET_MOVE_DIR_LL;
		break;
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		player->_direction = dfPACKET_MOVE_DIR_RR;
		break;
	}
	player->_move = dir;
	player->_x = x;
	player->_y = y;

	//해당 플레이어에 대한 움직임 정보 본인 제외 전체에게 send
	__int64 sessionA[6];
	int count = CollectOtherSessions(sessionID, sessionA);
	static_cast<FightProxy*>(static_cast<FightContents*>(_contents)->_proxy)->ProxySCMoveStart(sessionA, count, player->_id, dir, x, y);

}

void StubForFight::ProcCSMoveStop(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = FindPlayer(sessionID);
	if (player == nullptr)
	{
		return;
	}

	if (!IsPositionSyncValid(*player, x, y))
	{
		_server->Disconnect(sessionID);
		return;
	}
	//player 움직임 정보 변경
	switch (dir)
	{
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		player->_direction = dfPACKET_MOVE_DIR_LL;
		break;
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		player->_direction = dfPACKET_MOVE_DIR_RR;
		break;
	}
	player->_move = -1;
	player->_x = x;
	player->_y = y;


	//해당 플레이어에 대한 움직임 정보 본인 제외 전체에게 send
	__int64 sessionA[6];
	int count = CollectOtherSessions(sessionID, sessionA);
	(static_cast<FightProxy*>(static_cast<FightContents*>(_contents)->_proxy))->ProxySCMoveStop(sessionA, count, player->_id, dir, x, y);


}

void StubForFight::ProcCSAttack1(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = FindPlayer(sessionID);
	if (player == nullptr)
	{
		return;
	}

	if (!IsPositionSyncValid(*player, x, y))
	{
		_server->Disconnect(sessionID);
		return;
	}

	//해당 플레이어 공격 정보 처리
	UpdateActionState(*player, dir, x, y);

	//해당 플레이어 공격 정보 send()
	__int64 sessionA[6];
	int count = CollectOtherSessions(sessionID, sessionA);
	static_cast<FightProxy*>(static_cast<FightContents*>(_contents)->_proxy)->ProxySCAttack1(sessionA, count, player->_id, player->_direction, player->_x, player->_y);

	//데미지처리
	AttackPlayer(*player, CSATTACK1);

}

void StubForFight::ProcCSAttack2(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = FindPlayer(sessionID);
	if (player == nullptr)
	{
		return;
	}

	if (!IsPositionSyncValid(*player, x, y))
	{
		_server->Disconnect(sessionID);
		return;
	}
	//해당 플레이어 공격 정보 처리
	UpdateActionState(*player, dir, x, y);

	//해당 플레이어 공격 정보 send()
	__int64 sessionA[6];
	int count = CollectOtherSessions(sessionID, sessionA);
	static_cast<FightProxy*>(static_cast<FightContents*>(_contents)->_proxy)->ProxySCAttack2(sessionA, count, player->_id, player->_direction, player->_x, player->_y);

	//데미지처리
	AttackPlayer(*player, CSATTACK2);


}

void StubForFight::ProcCSAttack3(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = FindPlayer(sessionID);
	if (player == nullptr)
	{
		return;
	}

	if (!IsPositionSyncValid(*player, x, y))
	{
		_server->Disconnect(sessionID);
		return;
	}
	//해당 플레이어 공격 정보 처리
	UpdateActionState(*player, dir, x, y);

	//해당 플레이어 공격 정보 send()
	__int64 sessionA[6];
	int count = CollectOtherSessions(sessionID, sessionA);
	static_cast<FightProxy*>(static_cast<FightContents*>(_contents)->_proxy)->ProxySCAttack3(sessionA, count, player->_id, player->_direction, player->_x, player->_y);

	//데미지처리
	AttackPlayer(*player, CSATTACK3);


}

void StubForFight::ProcFightDefault(__int64 sessionID, CPacket packet)
{
	_server->Disconnect(sessionID);
}

void StubForFight::AttackPlayer(const Player& player, unsigned char type)
{
	Player* tgt = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.begin();
	for (; it != static_cast<FightContents*>(_contents)->_playerMap.end(); it++)
	{
		tgt = it->second;

		if (player._team == tgt->_team)
		{
			continue;
		}


		switch (type)
		{
		case CSATTACK1:
		{
			if (player._direction == dfPACKET_MOVE_DIR_LL)
			{
				if (tgt->_x <= player._x)
				{
					if ((player._x - tgt->_x) < dfATTACK1_RANGE_X && abs(player._y - tgt->_y) < dfATTACK1_RANGE_Y)
					{
						//hp처리 후 메시지 만들어 전체 send
						tgt->_hp -= ATTACK1DMG;
						if (tgt->_hp <= 0)
						{
							_server->Disconnect(tgt->_sessionID);
						}
						BroadcastDamage(player, *tgt);
						break;
					}
				}
			}
			else if (player._direction == dfPACKET_MOVE_DIR_RR)
			{

				if (tgt->_x >= player._x)
				{
					if ((tgt->_x - player._x) < dfATTACK1_RANGE_X && abs(tgt->_y - player._y) < dfATTACK1_RANGE_Y)
					{
						//_hp처리 후 메시지 만들어 전체 send
						tgt->_hp -= ATTACK1DMG;

						if (tgt->_hp <= 0)
						{
							_server->Disconnect(tgt->_sessionID);
						}
						BroadcastDamage(player, *tgt);
						break;
					}
				}
			}
			break;
		}
		case CSATTACK2:
		{
			if (player._direction == dfPACKET_MOVE_DIR_LL)
			{

				if (tgt->_x <= player._x)
				{
					if ((player._x - tgt->_x) < dfATTACK2_RANGE_X && abs(player._y - tgt->_y) < dfATTACK2_RANGE_Y)
					{
						//_hp처리 후 메시지 만들어 전체 send
						tgt->_hp -= ATTACK2DMG;

						if (tgt->_hp <= 0)
						{
							_server->Disconnect(tgt->_sessionID);
						}

						BroadcastDamage(player, *tgt);
						break;
					}
				}

			}
			else if (player._direction == dfPACKET_MOVE_DIR_RR)
			{

				if (tgt->_x >= player._x)
				{
					if ((tgt->_x - player._x) < dfATTACK2_RANGE_X && abs(tgt->_y - player._y) < dfATTACK2_RANGE_Y)
					{
						//_hp처리 후 메시지 만들어 전체 send
						tgt->_hp -= ATTACK2DMG;

						if (tgt->_hp <= 0)
						{
							_server->Disconnect(tgt->_sessionID);
						}

						BroadcastDamage(player, *tgt);
						break;
					}
				}
			}
			break;
		}
		case CSATTACK3:
		{
			if (player._direction == dfPACKET_MOVE_DIR_LL)
			{
				if (tgt->_x <= player._x)
				{
					if ((player._x - tgt->_x) < dfATTACK3_RANGE_X && abs(player._y - tgt->_y) < dfATTACK3_RANGE_Y)
					{
						//_hp처리 후 메시지 만들어 전체 send
						tgt->_hp -= ATTACK3DMG;

						if (tgt->_hp <= 0)
						{
							_server->Disconnect(tgt->_sessionID);
						}

						BroadcastDamage(player, *tgt);
						break;
					}
				}
			}
			else if (player._direction == dfPACKET_MOVE_DIR_RR)
			{

				if (tgt->_x >= player._x)
				{
					if ((tgt->_x - player._x) < dfATTACK3_RANGE_X && abs(tgt->_y - player._y) < dfATTACK3_RANGE_Y)
					{
						//_hp처리 후 메시지 만들어 전체 send
						tgt->_hp -= ATTACK3DMG;

						if (tgt->_hp <= 0)
						{
							_server->Disconnect(tgt->_sessionID);
						}

						BroadcastDamage(player, *tgt);
						break;
					}
				}

			}
			break;
		}
		}


	}
}

Player* StubForFight::FindPlayer(__int64 sessionID)
{
	FightContents* contents = static_cast<FightContents*>(_contents);
	auto it = contents->_playerMap.find(sessionID);
	if (it == contents->_playerMap.end())
	{
		return nullptr;
	}

	return it->second;
}

bool StubForFight::IsPositionSyncValid(const Player& player, unsigned short x, unsigned short y) const
{
	return abs(player._x - x) <= dfERROR_RANGE && abs(player._y - y) <= dfERROR_RANGE;
}

int StubForFight::CollectOtherSessions(__int64 sessionID, __int64(&sessionA)[6]) const
{
	int count = 0;
	auto* contents = static_cast<FightContents*>(_contents);

	for (auto it = contents->_playerMap.begin(); it != contents->_playerMap.end(); ++it)
	{
		if (it->first != sessionID)
		{
			sessionA[count++] = it->first;
		}
	}

	return count;
}

int StubForFight::CollectAllSessions(__int64(&sessionA)[6]) const
{
	int count = 0;
	auto* contents = static_cast<FightContents*>(_contents);

	for (auto it = contents->_playerMap.begin(); it != contents->_playerMap.end(); ++it)
	{
		sessionA[count++] = it->first;
	}

	return count;
}

void StubForFight::BroadcastDamage(const Player& attacker, const Player& target) const
{
	__int64 sessionA[6];
	int count = CollectAllSessions(sessionA);

	static_cast<FightProxy*>(static_cast<FightContents*>(_contents)->_proxy)
		->ProxySCDamage(sessionA, count, attacker._id, target._id, target._hp);
}

void StubForFight::UpdateActionState(Player& player, unsigned char dir, unsigned short x, unsigned short y) const
{
	player._direction = dir;
	player._x = x;
	player._y = y;
}
