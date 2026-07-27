#pragma once

#include <shared_mutex>
#include <winsock2.h>
#include <cstdint>
#include <chrono>

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
static constexpr std::uint16_t dfRANGE_MOVE_TOP = 50;
static constexpr std::uint16_t dfRANGE_MOVE_LEFT = 10;
static constexpr std::uint16_t dfRANGE_MOVE_RIGHT = 630;
static constexpr std::uint16_t dfRANGE_MOVE_BOTTOM = 470;

//-----------------------------------------------------------------
// 이동 오류체크 범위
//-----------------------------------------------------------------
static constexpr std::uint16_t dfERROR_RANGE = 50;

//---------------------------------------------------------------
// 공격범위.
//---------------------------------------------------------------
static constexpr std::uint16_t dfATTACK1_RANGE_X = 80;
static constexpr std::uint16_t dfATTACK2_RANGE_X = 90;
static constexpr std::uint16_t dfATTACK3_RANGE_X = 100;
static constexpr std::uint16_t dfATTACK1_RANGE_Y = 10;
static constexpr std::uint16_t dfATTACK2_RANGE_Y = 10;
static constexpr std::uint16_t dfATTACK3_RANGE_Y = 20;

//---------------------------------------------------------------
// 공격데미지.
//---------------------------------------------------------------
static constexpr std::uint8_t ATTACK1DMG = 10;
static constexpr std::uint8_t ATTACK2DMG = 30;
static constexpr std::uint8_t ATTACK3DMG = 50;

//-----------------------------------------------------------------
// 팀 위치
//-----------------------------------------------------------------
static constexpr std::uint16_t REDX = 40;
static constexpr std::uint16_t BLUEX = 600;
static constexpr std::uint16_t TEAMY = 180;

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

	std::uint32_t _id;
	std::uint8_t _direction;
	std::uint16_t _x=0;
	std::uint16_t _y=0;
	std::int8_t _hp=100;

	std::int8_t _move=-1;
	Team _team;			//1: red, 2: blue
	
	std::int32_t _contents;
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
	std::int64_t _matchID;
	char _winnerTeam;
	std::int64_t _red[3];
	std::int64_t _blue[3];
};

enum class DBRequestType
{
	SaveBattleResult
};

struct DBRequest
{
	DBRequestType _type;
	BattleResult _battleResult;
	std::chrono::steady_clock::time_point _queuedAt;
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