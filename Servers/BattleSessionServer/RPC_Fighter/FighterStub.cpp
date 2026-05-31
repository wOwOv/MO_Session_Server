#include "FighterDefine.h"
#include "FighterStub.h"
#include "CPacket.h"

void FightStub::ProcMessage(__int64 sessionID, CPacket packet)
{
unsigned char type;
packet >> type;
switch (type)
{
case CSMOVESTART:
{
unsigned char dir_=0;
 unsigned short x_=0;
 unsigned short y_=0;
packet>>dir_>>x_>>y_;
ProcCSMoveStart(sessionID,dir_,x_,y_);
break;
}
case CSMOVESTOP:
{
unsigned char dir_=0;
 unsigned short x_=0;
 unsigned short y_=0;
packet>>dir_>>x_>>y_;
ProcCSMoveStop(sessionID,dir_,x_,y_);
break;
}
case CSATTACK1:
{
unsigned char dir_=0;
 unsigned short x_=0;
 unsigned short y_=0;
packet>>dir_>>x_>>y_;
ProcCSAttack1(sessionID,dir_,x_,y_);
break;
}
case CSATTACK2:
{
unsigned char dir_=0;
 unsigned short x_=0;
 unsigned short y_=0;
packet>>dir_>>x_>>y_;
ProcCSAttack2(sessionID,dir_,x_,y_);
break;
}
case CSATTACK3:
{
unsigned char dir_=0;
 unsigned short x_=0;
 unsigned short y_=0;
packet>>dir_>>x_>>y_;
ProcCSAttack3(sessionID,dir_,x_,y_);
break;
}
default:
{
ProcFightDefault(sessionID,packet);
break;
}
}
}


void MatchStub::ProcMessage(__int64 sessionID, CPacket packet)
{
unsigned char type;
packet >> type;
switch (type)
{
default:
{
ProcMatchDefault(sessionID,packet);
break;
}
}
}


