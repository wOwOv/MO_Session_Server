#pragma once

#include <winsock2.h>
#include "LockFreeQueue(CAS).h"
#include "TlsMemoryPool.h"
#include "CPacket.h"
#include "IRPC.h"


enum TMessageType{ ENTER = 0, LEAVE, MESSAGE };
enum ContentNum {CONMOV=-1};

class Contents
{
	friend class ContentsServer;

public:
	Contents(__int32 contentsnum,__int32 frame);
	virtual ~Contents();

	unsigned long GetFPS();
	unsigned long GetLogic();

	void SetContentsNum(__int32 num);
	__int32 GetContentsNum();

	bool SendPacket(__int64 sessionID, CPacket packet);
	bool Disconnect(__int64 sessionID);
	void AttachStub(IStub* stub);
	IStub* DetachStub();
	void AttachProxy(IProxy* proxy);
	IProxy* DetachProxy();

protected:
	virtual void OnEnter(__int64 sessionID, void* extra) = 0;
	virtual void OnLeave(__int64 sessionID, void* extra) = 0;
	virtual void OnUpdate() = 0;



private:
	__int32 _contentsNum=0;
	__int32 _frame=0;				//ms단위, -1인 경우 프레임 필요없음을 명시한 것

	SRWLOCK _contentsKey;

private:
	unsigned long _fps;
	unsigned long _fpsCount;

	unsigned long _logic;
	unsigned long _logicCount;

protected:
	ContentsServer* _mServer;
	IStub* _stub=nullptr;
	IProxy* _proxy = nullptr;
};