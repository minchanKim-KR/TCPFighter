#pragma once
#include <Windows.h>
#include <list>
#include "Session.h"
#include "RingBuffer.h"

using namespace std;

class Proxy
{
public:
	int ServerReqCreateMyCharacter(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y,BYTE hp);
	int ServerReqCreateOtherCharacter(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y,BYTE hp);
	int ServerReqDeleteCharacter(int sessionId, BYTE sendMode, int id);
	int ClientReqMoveStart(int sessionId, BYTE sendMode, BYTE dir,short x,short y);
	int ServerReqMoveStart(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y);
	int ClientReqMoveStop(int sessionId, BYTE sendMode, BYTE dir,short x,short y);
	int ServerReqMoveStop(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y);
	int ClientReqAttack1(int sessionId, BYTE sendMode, BYTE dir,short x,short y);
	int ServerReqAttack1(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y);
	int ClientReqAttack2(int sessionId, BYTE sendMode, BYTE dir,short x,short y);
	int ServerReqAttack2(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y);
	int ClientReqAttack3(int sessionId, BYTE sendMode, BYTE dir,short x,short y);
	int ServerReqAttack3(int sessionId, BYTE sendMode, int id,BYTE dir,short x,short y);
	int ServerReqDamage(int sessionId, BYTE sendMode, int idAttacker,int idDefender,BYTE hpDefender);
};


