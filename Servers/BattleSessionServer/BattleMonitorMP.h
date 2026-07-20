#pragma once
#include "CPacket.h"
#include "MonitorProtocol.h"
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

__forceinline void MPGameDBTPS(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameDBMsg(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameAlloc(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_FIGHT_ALLOC;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameFree(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_FIGHT_FREE;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameCtrlQ(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_CTRL_Q;
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

__forceinline void MPGameFightFPSAvg(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_FIGHT_FPS_AVG;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameFightFPSMin(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_FIGHT_FPS_MIN;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPGameFightFPSMax(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_GAME_FIGHT_FPS_MAX;
	*packet << type << datatype << datavalue << timestamp;
}