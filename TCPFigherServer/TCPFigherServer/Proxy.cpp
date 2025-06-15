#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Proxy.h"
#include "PacketDefine.h"
#include "PacketHeader.h"
#include "SerializingBuffer.h"
#include "RingBuffer.h"
#include "SendPacket.h"
#include "Session.h"
#include "WriteLogTXT.h"
#include "PrintStackInfo.h"


#define SERIALIZING_BUF_FULLED 401


int Proxy::ServerReqCreateMyCharacter(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y,BYTE hp)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	sb << hp;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqCreateOtherCharacter(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y,BYTE hp)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	sb << hp;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqDeleteCharacter(int sessionId, BYTE sendMode,int id)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_DELETE_CHARACTER;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ClientReqMoveStart(int sessionId, BYTE sendMode,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_CS_MOVE_START;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqMoveStart(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_MOVE_START;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ClientReqMoveStop(int sessionId, BYTE sendMode,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_CS_MOVE_STOP;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqMoveStop(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_MOVE_STOP;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ClientReqAttack1(int sessionId, BYTE sendMode,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_CS_ATTACK1;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqAttack1(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_ATTACK1;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ClientReqAttack2(int sessionId, BYTE sendMode,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_CS_ATTACK2;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqAttack2(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_ATTACK2;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ClientReqAttack3(int sessionId, BYTE sendMode,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_CS_ATTACK3;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqAttack3(int sessionId, BYTE sendMode,int id,BYTE dir,short x,short y)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << id;
	sb << dir;
	sb << x;
	sb << y;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_ATTACK3;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
int Proxy::ServerReqDamage(int sessionId, BYTE sendMode,int idAttacker,int idDefender,BYTE hpDefender)
{
	SerializingBuffer sb;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	sb << idAttacker;
	sb << idDefender;
	sb << hpDefender;
	if(sb.Failed())
	{
		if (g_Members[sessionId]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s, SB에 데이터 삽입 실패(연결해제)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "BufferSize : %d, Capacity : %d, DataSize : %d",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());
			WriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());
			g_Members[sessionId]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ sessionId });
		}
		return SERIALIZING_BUF_FULLED;
	}
	Header header;
	header.byCode = HEADER_CODE;
	header.bySize = sb.GetDataSize();
	header.byType = dfPACKET_SC_DAMAGE;
	if(sendMode & SENDMODE_UNI)	{
		SendUnicast(sessionId, (char*)&header, sizeof(header));
		SendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	if(sendMode & SENDMODE_BROAD)	{
		SendBroadcast(sessionId, (char*)&header, sizeof(header));
		SendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());
	}
	else	{
		return -1;
	}
	return 0;
}
