#pragma once
#include "CPacket.h"
#include "windows.h"
#include "CommonProtocol.h"

__forceinline void MPChatRun(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN;
	*packet << type << datatype << datavalue << timestamp;
}
__forceinline void MPChatCpu(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU;
	*packet << type << datatype << datavalue << timestamp;

}
__forceinline void MPChatMem(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM;
	*packet << type << datatype << datavalue << timestamp;

}
__forceinline void MPChatSession(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_SESSION;
	*packet << type << datatype << datavalue << timestamp;
}
__forceinline void MPChatPlayer(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_PLAYER;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPUpdateTps(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPChatPacket(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL;
	*packet << type << datatype << datavalue << timestamp;
}

__forceinline void MPUpMsgPool(CPacket* packet, int datavalue, int timestamp)
{
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	BYTE datatype = dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL;
	*packet << type << datatype << datavalue << timestamp;
}