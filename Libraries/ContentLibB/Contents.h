#pragma once

#include <winsock2.h>
#include "LockFreeQueue(CAS).h"
#include "TlsMemoryPool.h"
#include "CPacket.h"
#include "IRPC.h"
#include <cstdint>


enum TMessageType{ ENTER = 0, LEAVE, MESSAGE };
enum ContentNum {CONMOV=-1};

class Contents
{
	friend class ContentsServer;

public:
	Contents(std::int32_t contentsnum,std::int32_t frame);
	virtual ~Contents();

	Contents(const Contents&) = delete;
	Contents& operator=(const Contents&) = delete;
	Contents(Contents&&) = delete;
	Contents& operator=(Contents&&) = delete;

	unsigned long GetFPS();
	unsigned long GetLogic();

	void SetContentsNum(std::int32_t num);
	std::int32_t GetContentsNum();

	bool SendPacket(std::int64_t sessionID, CPacket packet);
	bool Disconnect(std::int64_t sessionID);
	void AttachStub(IStub* stub);
	IStub* DetachStub();
	void AttachProxy(IProxy* proxy);
	IProxy* DetachProxy();

protected:
	virtual void OnEnter(std::int64_t sessionID, void* extra) = 0;  
	virtual void OnLeave(std::int64_t sessionID, void* extra) = 0;
	virtual void OnUpdate() = 0;
	virtual void OnShutDown()=0;		//컨텐츠를 종료하는 과정에서 락을 잡고 필요한 작업하는 용도


private:
	std::int32_t _contentsNum=0;
	std::int32_t _frame=0;				//ms단위, -1인 경우 프레임 필요없음을 명시한 것

	SRWLOCK _contentsKey;

private:
	unsigned long _fps;
	unsigned long _fpsCount;

	unsigned long _logic;
	unsigned long _logicCount;

protected:
	ContentsServer* _mServer=nullptr;	// // non-owning back-reference to ContentsServer
	IStub* _stub=nullptr;		// owning; deleted by derived content class
	IProxy* _proxy = nullptr;	// owning; deleted by derived content class
};