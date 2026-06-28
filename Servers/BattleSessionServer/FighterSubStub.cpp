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
	Player* player = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.find(sessionID);
	if (it != ((FightContents*)_contents)->_playerMap.end())
	{
		player = it->second;
	}
	else
	{ 
		return;
	}
	if (abs(player->_x - x) > dfERROR_RANGE || abs(player->_y - y) > dfERROR_RANGE)
	{
		_server->Disconnect(sessionID);
	}
	else
	{
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
		int count = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
		for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
		{
			if (cit->first != sessionID)
			{
				sessionA[count] = cit->first;
				count++;
			}
		}
		((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCMoveStart(sessionA, count, player->_id,dir, x, y);

	}
}

void StubForFight::ProcCSMoveStop(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.find(sessionID);
	if (it != ((FightContents*)_contents)->_playerMap.end())
	{
		player = it->second;
	}
	else
	{
		return;
	}
	if (abs(player->_x - x) > dfERROR_RANGE || abs(player->_y - y) > dfERROR_RANGE)
	{
		_server->Disconnect(sessionID);
	}
	else
	{
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
		int count = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
		for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
		{
			if (cit->first != sessionID)
			{
				sessionA[count] = cit->first;
				count++;
			}
		}
		((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCMoveStop(sessionA, count, player->_id,dir, x, y);

	}
}

void StubForFight::ProcCSAttack1(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.find(sessionID);
	if (it != ((FightContents*)_contents)->_playerMap.end())
	{
		player = it->second;
	}
	else
	{
		return;
	}
	if (abs(player->_x - x) > dfERROR_RANGE || abs(player->_y - y) > dfERROR_RANGE)
	{
		_server->Disconnect(sessionID);
	}
	else
	{
		//해당 플레이어 공격 정보 처리
		player->_direction = dir;
		player->_x = x;
		player->_y = y;

		//해당 플레이어 공격 정보 send()
		__int64 sessionA[6];
		int count = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
		for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
		{
			if (cit->first != sessionID)
			{
				sessionA[count] = cit->first;
				count++;
			}
		}
		((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCAttack1(sessionA, count, player->_id, player->_direction, player->_x, player->_y);

		//데미지처리
		AttackPlayer(*player, CSATTACK1);
	}
}

void StubForFight::ProcCSAttack2(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.find(sessionID);
	if (it != ((FightContents*)_contents)->_playerMap.end())
	{
		player = it->second;
	}
	else
	{
		return;
	}
	if (abs(player->_x - x) > dfERROR_RANGE || abs(player->_y - y) > dfERROR_RANGE)
	{
		_server->Disconnect(sessionID);
	}
	else
	{
		//해당 플레이어 공격 정보 처리
		player->_direction = dir;
		player->_x = x;
		player->_y = y;

		//해당 플레이어 공격 정보 send()
		__int64 sessionA[6];
		int count = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
		for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
		{
			if (cit->first != sessionID)
			{
				sessionA[count] = cit->first;
				count++;
			}
		}
		((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCAttack2(sessionA, count, player->_id,player->_direction, player->_x, player->_y);

		//데미지처리
		AttackPlayer(*player, CSATTACK2);

	}
}

void StubForFight::ProcCSAttack3(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)
{
	Player* player = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.find(sessionID);
	if (it != ((FightContents*)_contents)->_playerMap.end())
	{
		player = it->second;
	}
	else
	{
		return;
	}
	if (abs(player->_x - x) > dfERROR_RANGE || abs(player->_y - y) > dfERROR_RANGE)
	{
		_server->Disconnect(sessionID);
	}
	else
	{
		//해당 플레이어 공격 정보 처리
		player->_direction = dir;
		player->_x = x;
		player->_y = y;

		//해당 플레이어 공격 정보 send()
		__int64 sessionA[6];
		int count = 0;
		std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
		for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
		{
			if (cit->first != sessionID)
			{
				sessionA[count] = cit->first;
				count++;
			}
		}
		((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCAttack3(sessionA, count, player->_id,player->_direction, player->_x, player->_y);

		//데미지처리
		AttackPlayer(*player, CSATTACK3);

	}
}

void StubForFight::ProcFightDefault(__int64 sessionID, CPacket packet)
{
	_server->Disconnect(sessionID);
}

void StubForFight::AttackPlayer(const Player& player, unsigned char type)
{
	Player* tgt = nullptr;
	std::unordered_map<SessionID, Player*>::iterator it = ((FightContents*)_contents)->_playerMap.begin();
	for (; it != ((FightContents*)_contents)->_playerMap.end(); it++)
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
						__int64 sessionA[6];
						int count = 0;
						std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
						for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
						{
							sessionA[count] = cit->first;
							count++;
						}
						((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCDamage(sessionA, count, player._id, tgt->_id, tgt->_hp);
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
						__int64 sessionA[6];
						int count = 0;
						std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
						for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
						{
							sessionA[count] = cit->first;
							count++;
						}
						((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCDamage(sessionA, count, player._id, tgt->_id, tgt->_hp);
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

						__int64 sessionA[6];
						int count = 0;
						std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
						for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
						{
							sessionA[count] = cit->first;
							count++;
						}
						((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCDamage(sessionA, count, player._id, tgt->_id, tgt->_hp);
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

						__int64 sessionA[6];
						int count = 0;
						std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
						for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
						{
							sessionA[count] = cit->first;
							count++;
						}
						((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCDamage(sessionA, count, player._id, tgt->_id, tgt->_hp);
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

						__int64 sessionA[6];
						int count = 0;
						std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
						for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
						{
							sessionA[count] = cit->first;
							count++;
						}
						((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCDamage(sessionA, count, player._id, tgt->_id, tgt->_hp);
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

						__int64 sessionA[6];
						int count = 0;
						std::unordered_map<SessionID, Player*>::iterator cit = ((FightContents*)_contents)->_playerMap.begin();
						for (; cit != ((FightContents*)_contents)->_playerMap.end(); cit++)
						{
							sessionA[count] = cit->first;
							count++;
						}
						((FightProxy*)((FightContents*)_contents)->_proxy)->ProxySCDamage(sessionA, count, player._id, tgt->_id, tgt->_hp);
						break;
					}
				}

			}
			break;
		}
		}


	}
}
