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
	auto* contents = static_cast<FightContents*>(_contents);

	for (auto it = contents->_playerMap.begin(); it != contents->_playerMap.end(); ++it)
	{
		Player* target = it->second;

		if (player._team == target->_team)
		{
			continue;
		}

		if (!IsTargetInAttackRange(player, *target, type))
		{
			continue;
		}

		ApplyDamage(*target, GetAttackDamage(type));
		BroadcastDamage(player, *target);
		return;
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

int StubForFight::GetAttackDamage(unsigned char attackType) const
{
	switch (attackType)
	{
	case CSATTACK1: return ATTACK1DMG;
	case CSATTACK2: return ATTACK2DMG;
	case CSATTACK3: return ATTACK3DMG;
	default: return 0;
	}
}

bool StubForFight::IsTargetInAttackRange(const Player& attacker, const Player& target, unsigned char attackType) const
{
	int rangeX = 0;
	int rangeY = 0;

	switch (attackType)
	{
	case CSATTACK1:
		rangeX = dfATTACK1_RANGE_X;
		rangeY = dfATTACK1_RANGE_Y;
		break;
	case CSATTACK2:
		rangeX = dfATTACK2_RANGE_X;
		rangeY = dfATTACK2_RANGE_Y;
		break;
	case CSATTACK3:
		rangeX = dfATTACK3_RANGE_X;
		rangeY = dfATTACK3_RANGE_Y;
		break;
	default:
		return false;
	}

	if (attacker._direction == dfPACKET_MOVE_DIR_LL)
	{
		if (target._x > attacker._x)
		{
			return false;
		}

		return (attacker._x - target._x) < rangeX
			&& abs(attacker._y - target._y) < rangeY;
	}

	if (attacker._direction == dfPACKET_MOVE_DIR_RR)
	{
		if (target._x < attacker._x)
		{
			return false;
		}

		return (target._x - attacker._x) < rangeX
			&& abs(target._y - attacker._y) < rangeY;
	}

	return false;
}

void StubForFight::ApplyDamage(Player& target, int damage) const
{
	target._hp -= damage;
	if (target._hp <= 0)
	{
		_server->Disconnect(target._sessionID);
	}
}
