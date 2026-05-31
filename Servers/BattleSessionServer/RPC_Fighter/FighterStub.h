#pragma once
#include "IRPC.h"

class FightStub : public IStub
{
public:
virtual void ProcMessage(__int64 sessionID,CPacket packet) override;
protected:
virtual void ProcCSMoveStart(__int64 sessionID,unsigned char dir, unsigned short x, unsigned short y)=0;
virtual void ProcCSMoveStop(__int64 sessionID,unsigned char dir, unsigned short x, unsigned short y)=0;
virtual void ProcCSAttack1(__int64 sessionID,unsigned char dir, unsigned short x, unsigned short y)=0;
virtual void ProcCSAttack2(__int64 sessionID,unsigned char dir, unsigned short x, unsigned short y)=0;
virtual void ProcCSAttack3(__int64 sessionID,unsigned char dir, unsigned short x, unsigned short y)=0;
virtual void ProcFightDefault(__int64 sessionID, CPacket packet)=0;
};

class MatchStub : public IStub
{
public:
virtual void ProcMessage(__int64 sessionID,CPacket packet) override;
protected:
virtual void ProcMatchDefault(__int64 sessionID, CPacket packet)=0;
};

