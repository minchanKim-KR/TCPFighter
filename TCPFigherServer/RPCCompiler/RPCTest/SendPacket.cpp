#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Winmm.lib")
#include <WS2tcpip.h>

#include "SendPacket.h"
#include "Session.h"
#include "WriteLogTXT.h"
#include "PrintStackInfo.h"

int SendUnicast(int id, char* packet, int len)
{
	int enq_num;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];

	enq_num = g_Members[id]._sendBuf.Enqueue(packet, len);
	if (enq_num == 0)
	{
		// send ������ ������ ���� ����
		if (g_Members[id]._status == PS_VALID)
		{
			InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
			sprintf_s(err_buff1, sizeof err_buff1, "%s�� SendUnicast�� SendRingBuffer �뷮 �ʰ�.(���� ����)", ip_buffer);
			sprintf_s(err_buff2, sizeof err_buff2, "capacity : %d, datasize : %d", 
				g_Members[id]._sendBuf.GetCapacity(), 
				g_Members[id]._sendBuf.GetDataSize());

			WriteLog(err_buff1, err_buff2, getStackTrace(0).c_str());
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return RINGBUF_FULLED;
	}
	else if (enq_num != len)
	{
		// ������ ����
		sprintf_s(err_buff1, sizeof err_buff1, "input : %d, return Val : %d", len, enq_num);
		WriteLog("SendBuf.Enqueue() ���� ����.(input != return value)", err_buff1);
		return RINGBUF_ABNORMAL;
	}

	return 0;
}

// ID�� �ش��ϴ� CLINET�� ������ ��ο��� ��Ŷ ����.
int SendBroadcast(int id, char* packet, int len)
{
	bool ringbuf_fulled = false;
	char ip_buffer[20];
	char err_buff1[100];
	char err_buff2[100];
	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID && g_Members[i]._id != id)
		{
			int enq_num;
			enq_num = g_Members[i]._sendBuf.Enqueue(packet, len);
			if (enq_num == 0)
			{
				// send ������ ������ ���� ����
				if (g_Members[i]._status == PS_VALID)
				{
					InetNtopA(AF_INET, &g_Members[i].addr.sin_addr, ip_buffer, sizeof ip_buffer);
					sprintf_s(err_buff1, sizeof err_buff1, "%s�� SendBroadcast�� SendRingBuffer �뷮 �ʰ�.(���� ����)", ip_buffer);
					sprintf_s(err_buff2, sizeof err_buff2, "capacity : %d, datasize : %d",
						g_Members[i]._sendBuf.GetCapacity(),
						g_Members[i]._sendBuf.GetDataSize());

					WriteLog(err_buff1, err_buff2,getStackTrace(0).c_str());
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ i });
				}
				ringbuf_fulled = true;
			}
			else if (enq_num != len)
			{
				// ������ ����
				sprintf_s(err_buff1, sizeof err_buff1, "input : %d, return Val : %d", len, enq_num);
				WriteLog("SendBuf.Enqueue() ���� ����.(input != return value)", err_buff1);
				return RINGBUF_ABNORMAL;
			}
		}
	}

	if (ringbuf_fulled)
		return 0;
	else
		return RINGBUF_FULLED;
}