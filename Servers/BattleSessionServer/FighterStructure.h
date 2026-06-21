#pragma once

#include <shared_mutex>
#include <winsock2.h>


#define MATCH 1000000001				//contentsnum
#define SessionID __int64
#define ROOM 1000000000
#define	RED 1
#define BLUE 2
//-----------------------------------------------------------------
// 화면 이동영역
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	630
#define dfRANGE_MOVE_BOTTOM	470

//-----------------------------------------------------------------
// 이동 오류체크 범위
//-----------------------------------------------------------------
#define dfERROR_RANGE		50

//---------------------------------------------------------------
// 공격범위.
//---------------------------------------------------------------
#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

//---------------------------------------------------------------
// 공격데미지.
//---------------------------------------------------------------
#define ATTACK1DMG 10
#define ATTACK2DMG 30
#define ATTACK3DMG 50

//-----------------------------------------------------------------
// 팀 위치
//-----------------------------------------------------------------
#define REDX 40
#define BLUEX 600
#define TEAMY 180

#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7





struct Player
{
	SessionID _sessionID;
	SOCKADDR_IN _clientAddr;

	unsigned int _id;
	unsigned char _direction;
	unsigned short _x=0;
	unsigned short _y=0;
	signed char _hp=100;

	signed char _move=-1;
	char _team;			//1: red, 2: blue

	__int32 _contents;
};

struct Group
{
	SessionID _red[3];
	SessionID _blue[3];
};

enum CONTROLTYPE
{
	FIGHTALLOC = 1,
	FIGHTFREE,
	MATCHDEREGISTER
};

struct Control
{
	char _type;
	Group _group;
	void* _contents;
};

struct BattleResult
{
	__int64 _matchID;
	char _winnerTeam;
	__int64 _red[3];
	__int64 _blue[3];
};

enum class DBRequestType
{
	SaveBattleResult
};

struct DBRequest
{
	DBRequestType _type;
	BattleResult _battleResult;
};

class ReadLock
{
public:
	ReadLock(std::shared_mutex& mutex):_mutex(mutex)
	{
		_mutex.lock_shared();
	}
	~ReadLock()
	{
		_mutex.unlock_shared();
	}
private:
	std::shared_mutex& _mutex;
};

class WriteLock
{
public:
	WriteLock(std::shared_mutex& mutex) :_mutex(mutex)
	{
		_mutex.lock();
	}
	~WriteLock()
	{
		_mutex.unlock();
	}
private:
	std::shared_mutex& _mutex;
};