#include "FighterDefine.h"
#include "FighterProxy.h"
#include "ContentsServer.h"
#include "CPacket.h"

void FightProxy::ProxySCCreateMe(__int64* sessionA, int count, unsigned int id, unsigned char dir, unsigned short x, unsigned short y, unsigned char hp)
{
CPacket packet;
packet.Clear();
unsigned char type=SCCREATEME;
packet<<type<<id<<dir<<x<<y<<hp;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCCreateOther(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y,  unsigned char hp)
{
CPacket packet;
packet.Clear();
unsigned char type=SCCREATEOTHER;
packet<<type<<id<<dir<<x<<y<<hp;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCDelete(__int64* sessionA, int count, unsigned int id)
{
CPacket packet;
packet.Clear();
unsigned char type=SCDELETE;
packet<<type<<id;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCMoveStart(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y)
{
CPacket packet;
packet.Clear();
unsigned char type=SCMOVESTART;
packet<<type<<id<<dir<<x<<y;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCMoveStop(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y)
{
CPacket packet;
packet.Clear();
unsigned char type=SCMOVESTOP;
packet<<type<<id<<dir<<x<<y;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCAttack1(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y)
{
CPacket packet;
packet.Clear();
unsigned char type=SCATTACK1;
packet<<type<<id<<dir<<x<<y;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCAttack2(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y)
{
CPacket packet;
packet.Clear();
unsigned char type=SCATTACK2;
packet<<type<<id<<dir<<x<<y;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCAttack3(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y)
{
CPacket packet;
packet.Clear();
unsigned char type=SCATTACK3;
packet<<type<<id<<dir<<x<<y;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
void FightProxy::ProxySCDamage(__int64* sessionA, int count, unsigned int atk, unsigned int tgt, unsigned char hp)
{
CPacket packet;
packet.Clear();
unsigned char type=SCDAMAGE;
packet<<type<<atk<<tgt<<hp;
for (int i = 0; i < count; i++)
{
_server->SendPacket(sessionA[i], packet);
}
}
