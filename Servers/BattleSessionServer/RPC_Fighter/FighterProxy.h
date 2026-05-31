#pragma once
#include "IRPC.h"

class FightProxy : public IProxy
{
public:
void ProxySCCreateMe(__int64* sessionA, int count, unsigned int id, unsigned char dir, unsigned short x, unsigned short y, unsigned char hp);
void ProxySCCreateOther(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y,  unsigned char hp);
void ProxySCDelete(__int64* sessionA, int count, unsigned int id);
void ProxySCMoveStart(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y);
void ProxySCMoveStop(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y);
void ProxySCAttack1(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y);
void ProxySCAttack2(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y);
void ProxySCAttack3(__int64* sessionA, int count, unsigned int id,  unsigned char dir,  unsigned short x,  unsigned short y);
void ProxySCDamage(__int64* sessionA, int count, unsigned int atk, unsigned int tgt, unsigned char hp);
};
class MatchProxy : public IProxy
{
public:
};
