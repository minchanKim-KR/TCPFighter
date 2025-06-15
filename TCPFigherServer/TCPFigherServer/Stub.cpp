#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "PacketDefine.h"
#include "PacketHeader.h"
#include "RingBuffer.h"
#include "SerializingBuffer.h"
#include "RingBuffer.h"
#include "Session.h"
#include "WriteLogTXT.h"
#include "PrintStackInfo.h"
#include "Stub.h"
#include "StubHandler.h"


char g_recvStr[MAX_PAYLOAD_SIZE];


void Stub::RPC_PacketProcedure(int SessionId, Stub* handle)
{
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	RingBuffer& recvbuf = g_Members[SessionId]._recvBuf;
	Header h;
	SerializingBuffer sb;
	recvbuf.Dequeue((char*)&h, sizeof(Header));

	bool resizing;

	if (sb.GetBufferSize() < h.bySize)
		resizing = sb.Resize(h.bySize);
	if (resizing == false)
	{
		sprintf_s(err_buff1, "sb.BufferSize : % d", sb.GetBufferSize());
		WriteLog("SerializingBuffer.Resize 실패.", err_buff1,getStackTrace(1).c_str());

		if (g_Members[SessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[SessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "%s, SerializingBuffer Resizing 실패(연결해제)", ip_buffer);
			WriteLog(err_buff2);
			g_Members[SessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ SessionId });
		}
		return;
	}

	recvbuf.Dequeue((char*)sb.GetBufferPtr(), h.bySize);	sb.MoveWritePos(h.bySize);
	switch (h.byType)
		{
	case dfPACKET_SC_CREATE_MY_CHARACTER:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		BYTE hp;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		sb >> hp;
		handle->ServerReqCreateMyCharacter(id,dir,x,y,hp);
		break;
	}
	case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		BYTE hp;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		sb >> hp;
		handle->ServerReqCreateOtherCharacter(id,dir,x,y,hp);
		break;
	}
	case dfPACKET_SC_DELETE_CHARACTER:
		{
		int id;
		sb >> id;
		handle->ServerReqDeleteCharacter(id);
		break;
	}
	case dfPACKET_CS_MOVE_START:
		{
		BYTE dir;
		short x;
		short y;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ClientReqMoveStart(dir,x,y);
		break;
	}
	case dfPACKET_SC_MOVE_START:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ServerReqMoveStart(id,dir,x,y);
		break;
	}
	case dfPACKET_CS_MOVE_STOP:
		{
		BYTE dir;
		short x;
		short y;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ClientReqMoveStop(dir,x,y);
		break;
	}
	case dfPACKET_SC_MOVE_STOP:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ServerReqMoveStop(id,dir,x,y);
		break;
	}
	case dfPACKET_CS_ATTACK1:
		{
		BYTE dir;
		short x;
		short y;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ClientReqAttack1(dir,x,y);
		break;
	}
	case dfPACKET_SC_ATTACK1:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ServerReqAttack1(id,dir,x,y);
		break;
	}
	case dfPACKET_CS_ATTACK2:
		{
		BYTE dir;
		short x;
		short y;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ClientReqAttack2(dir,x,y);
		break;
	}
	case dfPACKET_SC_ATTACK2:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ServerReqAttack2(id,dir,x,y);
		break;
	}
	case dfPACKET_CS_ATTACK3:
		{
		BYTE dir;
		short x;
		short y;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ClientReqAttack3(dir,x,y);
		break;
	}
	case dfPACKET_SC_ATTACK3:
		{
		int id;
		BYTE dir;
		short x;
		short y;
		sb >> id;
		sb >> dir;
		sb >> x;
		sb >> y;
		handle->ServerReqAttack3(id,dir,x,y);
		break;
	}
	case dfPACKET_SC_DAMAGE:
		{
		int idAttacker;
		int idDefender;
		BYTE hpDefender;
		sb >> idAttacker;
		sb >> idDefender;
		sb >> hpDefender;
		handle->ServerReqDamage(idAttacker,idDefender,hpDefender);
		break;
	}
	default:
		{
		InetNtopA(AF_INET, &g_Members[SessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
		sprintf_s(err_buff1, sizeof err_buff1, "%s, 정의되지 않은 패킷타입.", ip_buffer);
		sprintf_s(err_buff2, sizeof err_buff2, "Packet Type : %d", h.byType);
		WriteLog(err_buff1, err_buff2, getStackTrace(1).c_str());

		if (g_Members[SessionId]._status == PS_VALID)
		{
			g_Members[SessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ SessionId });
		}
		break;
		}
	}
}
