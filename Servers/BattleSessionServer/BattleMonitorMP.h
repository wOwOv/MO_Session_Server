#pragma once
#include "CPacket.h"
#include "BattleMonitorProtocol.h"
#include <windows.h>

__forceinline void MPGameRun(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_SERVER_RUN;
	*packet << type << datatype << datavalue << timestamp;
}


__forceinline void MPGameCpu(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_SERVER_CPU;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameMem(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_SERVER_MEM;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameSes(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_SESSION;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameAuthP(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameGameP(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameAcp(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameRcv(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameSnd(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameAuthF(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameGameF(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGamePacket(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_PACKET_POOL;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameFightUsing(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_FIGHT_CONTENTS_USING;
	*packet << type << datatype << datavalue << timestamp;
}