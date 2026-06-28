#pragma once

#include <shared_mutex>
#include <winsock2.h>
#include <cstdint>

using SessionID = std::int64_t;
static constexpr std::int32_t MATCH = 1000000001;			//contentsnum
static constexpr std::int32_t ROOM = 1000000000;

enum class Team : std::uint8_t {
	RED = 1,
	BLUE = 2,
};
//-----------------------------------------------------------------
// 화면 이동영역
//-----------------------------------------------------------------
static constexpr unsigned short dfRANGE_MOVE_TOP = 50;
static constexpr unsigned short dfRANGE_MOVE_LEFT = 10;
static constexpr unsigned short dfRANGE_MOVE_RIGHT = 630;
static constexpr unsigned short dfRANGE_MOVE_BOTTOM = 470;

//-----------------------------------------------------------------
// 이동 오류체크 범위
//-----------------------------------------------------------------
static constexpr unsigned short dfERROR_RANGE = 50;

//---------------------------------------------------------------
// 공격범위.
//---------------------------------------------------------------
static constexpr unsigned short dfATTACK1_RANGE_X = 80;
static constexpr unsigned short dfATTACK2_RANGE_X = 90;
static constexpr unsigned short dfATTACK3_RANGE_X = 100;
static constexpr unsigned short dfATTACK1_RANGE_Y = 10;
static constexpr unsigned short dfATTACK2_RANGE_Y = 10;
static constexpr unsigned short dfATTACK3_RANGE_Y = 20;

//---------------------------------------------------------------
// 공격데미지.
//---------------------------------------------------------------
static constexpr unsigned char ATTACK1DMG = 10;
static constexpr unsigned char ATTACK2DMG = 30;
static constexpr unsigned char ATTACK3DMG = 50;

//-----------------------------------------------------------------
// 팀 위치
//-----------------------------------------------------------------
static constexpr unsigned short REDX = 40;
static constexpr unsigned short BLUEX = 600;
static constexpr unsigned short TEAMY = 180;

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
	Team _team;			//1: red, 2: blue

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