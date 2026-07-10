#pragma once
#include "CPacket.h"

class Contents;
class ContentsServer;
class IProxy;

void TraceProxyServerLink(const wchar_t* tag, ContentsServer* server, IProxy* proxy);

class IStub
{
public:
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
	static constexpr unsigned __int64 GUARD_A_VALUE = 0x1111222233334444ull;
	static constexpr unsigned __int64 GUARD_B_VALUE = 0x5555666677778888ull;

	void ConnectServer(void* server)
	{
		_server = static_cast<ContentsServer*>(server);
		// TraceProxyServerLink(L"[PROXY-CONNECTSERVER]", _server, this);
	}
public:
//protected:
	unsigned __int64 _guardA = GUARD_A_VALUE;
	ContentsServer* _server=nullptr;
	unsigned __int64 _guardB = GUARD_B_VALUE;
};

