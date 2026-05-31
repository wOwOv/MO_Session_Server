#include "Contents.h"
#include "ContentsServer.h"
#include <process.h>

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
}

IProxy* Contents::DetachProxy()
{
	IProxy* ret = _proxy;
	_proxy = nullptr;
	return ret;
}

