#pragma once
#include "CoreClient.h"

class MonitorClient:public CoreClient
{
public:
	MonitorClient(int serverno);
	~MonitorClient();

	virtual void OnConnect() override;
	virtual void OnRelease() override;
	virtual void OnMessage(CPacket packet) override;
	virtual void OnSecond() override;

private:
	int _serverNo;
};

