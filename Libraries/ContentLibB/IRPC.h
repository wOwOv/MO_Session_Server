#pragma once
#include "CPacket.h"

class Contents;
class ContentsServer;

class IStub
{
public:
	virtual void ProcMessage(__int64 sessionID, CPacket packet) = 0;
	void ConnectServer(void* server)
	{
		_server = (ContentsServer*)server;
	}
	void ConnectContents(void* contents)
	{
		_contents = (Contents*)contents;
	}

protected:
	ContentsServer* _server;
	Contents* _contents;
};
class IProxy
{
public:
	void ConnectServer(void* server)
	{
		_server = (ContentsServer*)server;
	}

protected:
	ContentsServer* _server;
};

