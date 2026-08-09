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
	void AttackPlayer(const Player& player, unsigned char type);
	Player* FindPlayer(__int64 sessionID);
	bool IsPositionSyncValid(const Player& player, unsigned short x, unsigned short y) const;
	bool IsValidMoveDirection(unsigned char dir);
	bool CanHandleMoveRequest(const Player& player, std::uint32_t now) const;
	bool CanHandleAttack(const Player& player, std::uint32_t now) const;

	int CollectOtherSessions(__int64 sessionID, __int64(&sessionA)[6]) const;
	int CollectAllSessions(__int64(&sessionA)[6]) const;
	void BroadcastDamage(const Player& attacker, const Player& target) const;
	void UpdateActionState(Player& player, unsigned char dir, unsigned short x, unsigned short y) const;

	int GetAttackDamage(unsigned char attackType) const;
	bool IsTargetInAttackRange(const Player& attacker, const Player& target, unsigned char attackType) const;
	void ApplyDamage(Player& target, int damage) const;

private:
};