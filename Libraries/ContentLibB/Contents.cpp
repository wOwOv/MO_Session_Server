#include "Contents.h"
#include "ContentsServer.h"
#include "Logger.h"
#include <process.h>

void TraceProxyServerLink(const wchar_t* tag, ContentsServer* server, IProxy* proxy)
{
	LOG(L"ProxyTrace", LVSYSTEM,
		L"%s server=%p proxy=%p proxy_server=%p guardA=%llX guardB=%llX",
		tag,
		server,
		proxy,
		proxy ? proxy->_server : nullptr,
		proxy ? proxy->_guardA : 0ull,
		proxy ? proxy->_guardB : 0ull);
}

Contents::Contents(__int32 contentsnum, __int32 frame)
{
	_contentsNum =contentsnum;
	_frame = frame;

	InitializeSRWLock(&_contentsKey);
}

Contents::~Contents()
{
}


unsigned long Contents::GetFPS()
{
	return _fps;
}

unsigned long Contents::GetLogic()
{
	return _logic;
}

void Contents::SetContentsNum(__int32 num)
{
	_contentsNum = num;
}

__int32 Contents::GetContentsNum()
{
	return _contentsNum;
}

bool Contents::SendPacket(__int64 sessionID, CPacket packet)
{
	if (_mServer == nullptr)
	{
		return false;
	}
	else
	{
		_mServer->SendPacket(sessionID, packet);
		return true;
	}
}

bool Contents::Disconnect(__int64 sessionID)
{
	if (_mServer == nullptr)
	{
		return false;
	}
	else
	{
		_mServer->Disconnect(sessionID);
		return true;
	}
}

void Contents::AttachStub(IStub* stub)
{
	_stub = stub;
	_stub->ConnectContents(this);
}

IStub* Contents::DetachStub()
{
	IStub* ret = _stub;
	_stub = nullptr;
	return ret;
}

void Contents::AttachProxy(IProxy* proxy)
{
	_proxy = proxy;
	LOG(L"ProxyTrace", LVSYSTEM,
		L"[CONTENTS-ATTACH-PROXY] contents=%p num=%d proxy=%p proxy_server=%p guardA=%llX guardB=%llX mserver=%p",
		this,
		GetContentsNum(),
		_proxy,
		_proxy ? _proxy->_server : nullptr,
		_proxy ? _proxy->_guardA : 0ull,
		_proxy ? _proxy->_guardB : 0ull,
		_mServer);
}

IProxy* Contents::DetachProxy()
{
	IProxy* ret = _proxy;
	LOG(L"ProxyTrace", LVSYSTEM,
		L"[CONTENTS-DETACH-PROXY] contents=%p num=%d proxy=%p proxy_server=%p guardA=%llX guardB=%llX mserver=%p",
		this,
		GetContentsNum(),
		ret,
		ret ? ret->_server : nullptr,
		ret ? ret->_guardA : 0ull,
		ret ? ret->_guardB : 0ull,
		_mServer);
	_proxy = nullptr;
	return ret;
}
