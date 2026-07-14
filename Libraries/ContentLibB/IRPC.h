#pragma once
#include "CPacket.h"

class Contents;
class ContentsServer;

class IStub
{
public:
	virtual ~IStub() = default;
	virtual void ProcMessage(__int64 sessionID, CPacket packet) = 0;
	void ConnectServer(void* server)
	{
		_server = static_cast<ContentsServer*>(server);
	}
	void ConnectContents(void* contents)
	{
		_contents = static_cast<Contents*>(contents);
	}

protected:
	ContentsServer* _server=nullptr;
	Contents* _contents=nullptr;
};
class IProxy
{
public:
	virtual ~IProxy() = default;
	void ConnectServer(void* server)
	{
		_server = static_cast<ContentsServer*>(server);
	}

protected:
	ContentsServer* _server=nullptr;
};

