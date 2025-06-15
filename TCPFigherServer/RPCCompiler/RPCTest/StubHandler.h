#pragma once
#include <list>
#include "Stub.h"

using namespace std;

class StubHandler : public Stub
{
public:
	static int _now_id;
public:
	int ServerReqCreateMyCharacter(int id, BYTE dir, short x, short y, BYTE hp);
	int ServerReqCreateOtherCharacter(int id, BYTE dir, short x, short y, BYTE hp);
	int ServerReqDeleteCharacter(int id);
	int ClientReqMoveStart(BYTE dir, short x, short y);
	int ServerReqMoveStart(int id, BYTE dir, short x, short y);
	int ClientReqMoveStop(BYTE dir, short x, short y);
	int ServerReqMoveStop(int id, BYTE dir, short x, short y);
	int ClientReqAttack1(BYTE dir, short x, short y);
	int ServerReqAttack1(int id, BYTE dir, short x, short y);
	int ClientReqAttack2(BYTE dir, short x, short y);
	int ServerReqAttack2(int id, BYTE dir, short x, short y);
	int ClientReqAttack3(BYTE dir, short x, short y);
	int ServerReqAttack3(int id, BYTE dir, short x, short y);
	int ServerReqDamage(int idAttacker, int idDefender, BYTE hpDefender);
};
