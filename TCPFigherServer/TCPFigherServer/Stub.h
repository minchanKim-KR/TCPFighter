#pragma once
#include <Windows.h>
#include <list>
#include "Session.h"
#include "RingBuffer.h"
#include "PacketHeader.h"

using namespace std;


class Stub
{
public:
	static void RPC_PacketProcedure(int SessionId,Stub* handle);

	virtual int ServerReqCreateMyCharacter(int id,BYTE dir,short x,short y,BYTE hp) = 0;
	virtual int ServerReqCreateOtherCharacter(int id,BYTE dir,short x,short y,BYTE hp) = 0;
	virtual int ServerReqDeleteCharacter(int id) = 0;
	virtual int ClientReqMoveStart(BYTE dir,short x,short y) = 0;
	virtual int ServerReqMoveStart(int id,BYTE dir,short x,short y) = 0;
	virtual int ClientReqMoveStop(BYTE dir,short x,short y) = 0;
	virtual int ServerReqMoveStop(int id,BYTE dir,short x,short y) = 0;
	virtual int ClientReqAttack1(BYTE dir,short x,short y) = 0;
	virtual int ServerReqAttack1(int id,BYTE dir,short x,short y) = 0;
	virtual int ClientReqAttack2(BYTE dir,short x,short y) = 0;
	virtual int ServerReqAttack2(int id,BYTE dir,short x,short y) = 0;
	virtual int ClientReqAttack3(BYTE dir,short x,short y) = 0;
	virtual int ServerReqAttack3(int id,BYTE dir,short x,short y) = 0;
	virtual int ServerReqDamage(int idAttacker,int idDefender,BYTE hpDefender) = 0;
};


