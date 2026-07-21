#include "MonitorClient.h"
#include "MonitorProtocol.h"
#include "CPacket.h"
#include <iostream>

MonitorClient::MonitorClient(int serverno)
{
	_serverNo = serverno;
}

MonitorClient::~MonitorClient()
{
}

void MonitorClient::OnConnect()
{
	printf("ConnectedToMonitorServer\n");
	WORD type = en_PACKET_SS_MONITOR_LOGIN;
	CPacket packet;
	packet << type << _serverNo;
	SendPacket(packet);
}

void MonitorClient::OnRelease()
{
}

void MonitorClient::OnMessage(CPacket packet)
{
}

void MonitorClient::OnSecond()
{
}
