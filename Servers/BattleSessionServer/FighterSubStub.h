#pragma once
#include "FighterStub.h"
#include "FighterContents.h"
#include "FighterServer.h"

class StubForMatch :public MatchStub
{
public:
	virtual void ProcMatchDefault(__int64 sessionID, CPacket packet)override;
	
private:
};

class StubForFight :public FightStub
{
public:
	virtual void ProcCSMoveStart(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)override;
	virtual void ProcCSMoveStop(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)override;
	virtual void ProcCSAttack1(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)override;
	virtual void ProcCSAttack2(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)override;
	virtual void ProcCSAttack3(__int64 sessionID, unsigned char dir, unsigned short x, unsigned short y)override;
	virtual void ProcFightDefault(__int64 sessionID, CPacket packet)override;

private:
	void AttackPlayer(const Player* player, unsigned char type);

private:
};