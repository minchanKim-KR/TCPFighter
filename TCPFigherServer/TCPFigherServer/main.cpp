#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <Windows.h>
#include <queue>

#include "SendPacket.h"
#include "Session.h"
#include "PacketDefine.h"
#include "WriteLogTXT.h"
#include "PrintStackInfo.h"

#include "Proxy.h"
#include "Stub.h"
#include "StubHandler.h"

using namespace std;

// Select Model�� �⺻������ set�� �ִ� 64���� ������ �˻� ����.

#define SERVER_PORT 5000

// Content�� ���� ������ ���� ��
#define TOTAL_PLAYER 60
#define HEADER_SIZE 3

// DATA
int g_numMembers; // ������ ��

//FPS (�Ⱦ���)
//DWORD old_for_1s;
//DWORD old;
//DWORD deltaTime;
//int fps;
//int fps_for_print;

//PACKET
// �ش� �����ӿ� ó���� ��Ŷ ��
int g_num_packet;

void Init();
bool Update(SOCKET& listenSock);
void DeleteSockets();

void UpdateLogic();
void MovingUpdate(int id);
void Att1Update(int id);
void Att2Update(int id);
void Att3Update(int id);

int SetId();
void PlayerIsJoined(SOCKET& clientSock, SOCKADDR_IN& clientAddr);

void main(void)
{
#pragma region InitListen
	int retval;
	int ret_create_socket;
	int ret_bind;
	int ret_listen;
	int ret_ioctl;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		cout << "socket() INVALID_SOCKET" << endl;
		ret_create_socket = GetLastError();
		cout << "Error : " << ret_create_socket << endl;
		return;
	}

	// 0.0.0.0:5000 �� ���� Listen 
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof serveraddr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);

	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof serveraddr);
	if (retval == SOCKET_ERROR)
	{
		// ��ȿ���� ���� ����
		// �̹� ���ε� �� ����
		// �̹� ������� ��Ʈ(���� ip + port ������ �� ���ϸ� ��밡��)

		cout << "Error : bind()" << endl;
		ret_bind = GetLastError();
		cout << "ErrCode : " << ret_bind << endl;
		return;
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		// ��ȿ���� ���� ����

		wcout << L"Error : listen()" << endl;
		ret_listen = GetLastError();
		wcout << L"ErrCode : " << ret_listen << endl;
		return;
	}

	//////////////////////////////////////////////////////
	// Non Blocking Socket���� ��ȯ
	//////////////////////////////////////////////////////
	u_long on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)
	{
		wcout << L"ioctlsocket() SOCKET_ERROR" << endl;
		ret_ioctl = GetLastError();
		wcout << L"Error : " << ret_ioctl << endl;
		return;
	}
#pragma endregion InitListen

	Init();
	timeBeginPeriod(1);

	int delta_time;
	int old_time = timeGetTime();
	int sleep_time;

	while (1)
	{
		if (!Update(listen_sock))
			break;

		//50 fps ����
		delta_time = timeGetTime() - old_time;
		sleep_time = 20 - delta_time;
		if (sleep_time > 0)
		{
			Sleep(sleep_time);
		}
		old_time += 20;
	}

	timeEndPeriod(1);
}

void Init()
{
	for (int i = 0;i < TOTAL_PLAYER;i++)
		g_Members->_status = PS_EMPTY;

	g_num_packet = 0;
	g_numMembers = 0;
}

// 
bool Update(SOCKET& listenSock)
{
	int i;
	int retval;
	int ret_select;
	int ret_ioctl;

	int id;
	// nonblocking socket
	u_long on = 1;

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	FD_SET rset;
	FD_SET wset;
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	addrlen = sizeof clientaddr;

	//////////////////////////////////////////////////////
	// RECV Select
	//////////////////////////////////////////////////////

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	// Read Set
	FD_SET(listenSock, &rset);
	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID)
		{
			FD_SET(g_Members[i].sock, &rset);
		}
	}

	retval = select(0, &rset, &wset, NULL, &tv);
	if (retval == SOCKET_ERROR)
	{
		ret_select = GetLastError();
		printf_s("Select For Recv, ErrorCode : %d.\n", ret_select);
		return false;
	}

	//////////////////////////////////////////////////////
	// connect 
	//////////////////////////////////////////////////////
	if (FD_ISSET(listenSock, &rset))
	{
		client_sock = accept(listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			// LISTEN�� ���� ���
			cout << "Error : accept()" << endl;
			cout << "ErrCode : " << GetLastError() << endl;
			return false;
		}

		// nonblocking socket
		retval = ioctlsocket(client_sock, FIONBIO, &on);
		if (retval == SOCKET_ERROR)
		{
			wcout << L"ioctlsocket() SOCKET_ERROR" << endl;
			ret_ioctl = GetLastError();
			wcout << L"Error : " << ret_ioctl << endl;
			return false;
		}

		PlayerIsJoined(client_sock, clientaddr);
	}

	//////////////////////////////////////////////////////
	// RECV
	//////////////////////////////////////////////////////
	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		// �а����� �����Ƿ� ���� ��ȣ�� ��ĥ������.
		if (g_Members[i]._status == PS_VALID && FD_ISSET(g_Members[i].sock, &rset))
		{
			int now_cap = g_Members[i]._recvBuf.GetCapacity();
			int num_direct_enq = g_Members[i]._recvBuf.DirectEnqueueSize();
			////////////////////////////////////////////////////////////////
			// 1. ���� ������ ������ Recv���� ����.
			////////////////////////////////////////////////////////////////
			if (now_cap < 1)
			{
				continue;
			}
			////////////////////////////////////////////////////////////////
			// 2. Select�� �����̹Ƿ� Recv�õ�.
			////////////////////////////////////////////////////////////////
			retval = recv(g_Members[i].sock
				, g_Members[i]._recvBuf.GetHeadPtr()
				, num_direct_enq
				, 0);
			if (retval == SOCKET_ERROR)
			{
				// RST (disconnect)
				if (GetLastError() == 10054)
				{

				}
				// �������� ���� ����
				else
				{
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "Recv()���� �� �� ���� ����.\n ERRORCODE : %d", GetLastError());
					cout << "�̻󿡷�" << endl;
					WriteLog(err_buff, "Recv Error");
				}

				g_Members[i]._status = PS_INVALID;
				g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
				continue;
			}
			// FIN (disconnect)
			else if (retval == 0)
			{
				g_Members[i]._status = PS_INVALID;
				g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
				continue;
			}
			// ���� ����
			else
			{
				g_Members[i]._recvBuf.MoveTail(retval);
				g_Members[i]._recvBuf._dataSize += retval;
			}
			////////////////////////////////////////////////////////////////////////////
			// 3. ringbuffer�� direct ������ ä��, �����۰� �� ���� ����.
			////////////////////////////////////////////////////////////////////////////
			if (retval == num_direct_enq && !g_Members[i]._recvBuf.IsFull())
			{
				//cout << "recv���� �ѹ��� ������. " << endl;
				//cout << "retval == num direct enqsize :" << retval << endl;
				//cout << "Head : " << g_Members[i]._recvBuf._head << endl;
				//cout << "Tail : " << g_Members[i]._recvBuf._tail << endl;
				//cout << endl;
				retval = recv(g_Members[i].sock
					, g_Members[i]._recvBuf.GetTailPtr()
					, g_Members[i]._recvBuf.DirectEnqueueSize()
					, 0);
				if (retval == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
				{
					// recv�� ������ ����.
				}
				else if (retval == SOCKET_ERROR)
				{
					// RST (disconnect)
					if (GetLastError() == 10054)
					{
						
					}
					// �������� ���� ����
					else
					{
						char err_buff[100];
						sprintf_s(err_buff, sizeof err_buff, "Recv()���� �� �� ���� ����.\n ERRORCODE : %d", GetLastError());
						cout << "�̻󿡷�" << endl;
						WriteLog(err_buff, "Recv Error");
					}

					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
					continue;
				}
				// FIN (disconnect)
				else if (retval == 0)
				{
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
					continue;
				}
				// ���� ����
				else
				{
					g_Members[i]._recvBuf.MoveTail(retval);
					g_Members[i]._recvBuf._dataSize += retval;
				}

				cout << "�߰� ������ ���� " << endl;
				cout << "retval : " << retval << endl;
				cout << "Head : " << g_Members[i]._recvBuf._head << endl;
				cout << "Tail : " << g_Members[i]._recvBuf._tail << endl;
				cout << endl;
			}

			g_num_packet++;
			
			//////////////////////////////////////////////////////
			// ���Ź��ۿ� �ִ� ��Ŷ ó��
			//////////////////////////////////////////////////////
			char ip_buffer[20];
			while (1)
			{
				Header h;
				int num_deq;

				//////////////////////////////////////////////////////
				// Header �˻�
				//////////////////////////////////////////////////////
				// ����۾�
				if (g_Members[i]._recvBuf.GetDataSize() < sizeof Header)
					break;

				num_deq = g_Members[i]._recvBuf.Peek((char*)&h, sizeof Header);
				if (num_deq != sizeof Header)
				{
					// ������ ���� �α�
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
					WriteLog(err_buff);

					return false;
				}

				// ������ ����ΰ�?
				if (h.byCode != 0x89 || h.bySize > MAX_PAYLOAD_SIZE)
				{
					// ��� �̻�
					InetNtopA(AF_INET, &g_Members[i].addr.sin_addr, ip_buffer, sizeof ip_buffer);
					cout << "[Abnormal] " << ip_buffer << ":" << ntohs(g_Members[i].addr.sin_port) <<
						"Header, byCode : " << h.byCode << "bySize : " << h.bySize << endl << endl;

					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
					break;
				}

				// payload�� ���� ���ۿ� �ֳ�?
				if (g_Members[i]._recvBuf.GetDataSize() < sizeof Header + h.bySize)
					break;

				//////////////////////////////////////////////////////
				// PAYLOAD
				//////////////////////////////////////////////////////

				// PacketProcedure(Ringbuf& rb);

				// 1. Header�� payload�� ������.
				// Header, SerializingBuffer.PutData <- Ringbuf.Dequeue

				// 2. �Ű����� (char*, list<>&�� �����Ҵ�?) -> �ϴ� �����Ҵ�.
				// 3. ����Ÿ���� �׳� sb���� �����.
				// 4. ������ �ñ״��ĸ� ȣ���Ŵ
				// 5. �ش� �ñ״��ĸ� ������ ���� iStub recv_stb = new Stub�� ���� stb.Mk_packet(...);�� ȣ��
						
				StubHandler handle;
				handle._now_id = i;
				Stub::RPC_PacketProcedure(i, &handle);
				
			}
		}
	}

	//////////////////////////////////////////////////////
	// Logic
	////////////////////////////////////////////////////// 
	UpdateLogic();

	//////////////////////////////////////////////////////
	// Delete
	//////////////////////////////////////////////////////
	DeleteSockets();
	
	//////////////////////////////////////////////////////
	// SEND Select
	//////////////////////////////////////////////////////

	// ���� : select�� �ƹ��͵� �ȳ����� 
	// (��� ���� COUNT�� 0�̸�) ERROR 10022 �Ű���������
	FD_ZERO(&rset);
	FD_SET(listenSock, &rset);

	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID && g_Members[i]._sendBuf.GetDataSize() > 0)
		{
			FD_SET(g_Members[i].sock, &wset);
		}
	}

	retval = select(0, &rset, &wset, NULL, &tv);
	if (retval == SOCKET_ERROR)
	{
		// 10022 �Ű����� ����, rset, wset count�� ���� 0��ä�� ����Ǹ� error
		ret_select = GetLastError();
		printf_s("send select ErrorCode : %d.\n", ret_select);
		return false;
	}

	//////////////////////////////////////////////////////
	// SEND
	//////////////////////////////////////////////////////
	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		// �а����� �����Ƿ� ���� ��ȣ�� ��ĥ������.
		if (g_Members[i]._status == PS_VALID && FD_ISSET(g_Members[i].sock, &wset))
		{
			int retval;
			
			// 1. send�� �õ�
			retval = send(g_Members[i].sock
				, g_Members[i]._sendBuf.GetHeadPtr()
				, g_Members[i]._sendBuf.DirectDequeueSize()
				, 0);			
			if (retval == SOCKET_ERROR)
			{
				//select�Ǿ��� ������ ���۰� �� �� wouldblock �ɸ� ���� ����.

				int send_error = GetLastError();

				// RST (disconnect) WSAECONNRESET (10054)
				if (send_error == WSAECONNRESET)
				{
					
				}
				// �������� ���� ����
				else
				{
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "Recv()���� �� �� ���� ����.\n ERRORCODE : %d", GetLastError());
					cout << "�̻󿡷�" << endl;
					WriteLog(err_buff, "Recv Error");
				}

				if (g_Members[i]._status == PS_VALID)
				{
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ i });
				}
				continue;
				// SEND�õ�, ���� �������� CLOSESOCKET QUEUEING
			}

			// 2. directsize��ŭ �ѹ��� ����. �������� ���� �����ӿ� ����
			g_Members[i]._sendBuf._dataSize -= retval;
			g_Members[i]._sendBuf.MoveHead(retval);

			// 3. �̹��� ���� ��������, clearbuffer�� ������ �ʱ�ȭ.
			//if (g_Members[i]._sendBuf.IsEmpty())
			//{
			//	g_Members[i]._sendBuf.ClearBuffer();
			//}
		}
	}
	return true;
}

// �÷��̾� ������ ���� ó��
void PlayerIsJoined(SOCKET& clientSock, SOCKADDR_IN& clientAddr)
{
	char ip_buffer[20];
	int id = SetId();
	if (id == -1)
	{
		// ���� ����
		closesocket(clientSock);
		return;
	}

	g_Members[id]._status = PS_VALID;
	g_Members[id].sock = clientSock;
	g_Members[id].addr = clientAddr;
	g_Members[id]._id = id;
	g_Members[id]._tStamp = GetTickCount64();

	g_Members[id]._player._act = ACT_IDLE;
	g_Members[id]._player._dir = dfPACKET_MOVE_DIR_LL;
	g_Members[id]._player._x = (rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT)) + dfRANGE_MOVE_LEFT;
	g_Members[id]._player._y = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP)) + dfRANGE_MOVE_TOP;;
	g_Members[id]._player._hp = 100;
	g_Members[id]._is_payload = false;
	g_numMembers++;

	InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
	cout << "[CON] " << ip_buffer << ":" << ntohs(g_Members[id].addr.sin_port) <<
		"���� ����.(������ : " << g_numMembers << ")" << endl << endl;

	Proxy proxy;
	// 1. client id �Ҵ� �� ĳ���� ����
	proxy.ServerReqCreateMyCharacter(id, SENDMODE_UNI,
		id,
		g_Members[id]._player._dir,
		g_Members[id]._player._x,
		g_Members[id]._player._y,
		g_Members[id]._player._hp);
	

	// 2. �ٸ� �����ڿ��� �ش��÷��̾� ĳ���� ����
	proxy.ServerReqCreateOtherCharacter(id, SENDMODE_BROAD,
		id,
		g_Members[id]._player._dir,
		g_Members[id]._player._x,
		g_Members[id]._player._y,
		g_Members[id]._player._hp);


	// ���⼭ �÷��̾� ���±��� ��������.
	// 3. �����ڿ��� ���� �ο��鿡 ���� ��ġ, ���� ����
	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		// ���� �����ϰų�, ���� ������� ����.
		if (g_Members[i]._status != PS_EMPTY && g_Members[i]._id != id)
		{
			proxy.ServerReqCreateOtherCharacter(id, SENDMODE_UNI,
				g_Members[i]._id,
				g_Members[i]._player._dir,
				g_Members[i]._player._x,
				g_Members[i]._player._y,
				g_Members[i]._player._hp);

			// �ִϸ��̼� ����ȭ�� �Ű澲�� �ʴ´�.

			if (g_Members[i]._player._act == ACT_MOVING)
			{
				proxy.ServerReqMoveStart(id, SENDMODE_UNI,
					g_Members[i]._id,
					g_Members[i]._player._dir,
					g_Members[i]._player._x,
					g_Members[i]._player._y);
			}
		}
	}
}

// �ڸ��� �ִ��� �˻��ϰ�, ID�ذ�
int SetId()
{
	// HACK : ��ġ�� �迭�� �ε����� ID�� ����.
	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_EMPTY)
		{
			return i;
		}
	}

	// �ڸ��� ������ ���� ����.
	return -1;
}

void DeleteSockets()
{
	// �������ڸ��� �����
	// connect���� �ѹ�, read���� rst�� ���ѹ�
	Proxy proxy;

	while (!g_DeleteQueue.empty())
	{
		char ip_buffer[20];

		int id = g_DeleteQueue.front()._id;
		g_DeleteQueue.pop();

		g_Members[id]._recvBuf.ClearBuffer();
		g_Members[id]._sendBuf.ClearBuffer();
		closesocket(g_Members[id].sock);
		g_Members[id]._status = PS_EMPTY;
		g_numMembers--;

		InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
		cout << "[CLS] " << ip_buffer << ":" << ntohs(g_Members[id].addr.sin_port) <<
			"���� ���� ����. (������ : " << g_numMembers << ")" << endl << endl;
	
		// ���� ĳ���� ����
		proxy.ServerReqDeleteCharacter(id, SENDMODE_UNI | SENDMODE_BROAD, id);
	}
}

void UpdateLogic()
{
	for (int i = 0; i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status != PS_VALID)
			continue;

		Player_Action act = g_Members[i]._player._act;
		switch (act)
		{
		case ACT_MOVESTART:
			g_Members[i]._player._act = ACT_MOVING;
			break;
		case ACT_MOVING:
			MovingUpdate(i);
			break;
		case ACT_IDLE:
			break;
		case ACT_ATT1:
			Att1Update(i);
			g_Members[i]._player._act = ACT_IDLE;
			break;
		case ACT_ATT2:
			Att2Update(i);
			g_Members[i]._player._act = ACT_IDLE;
			break;
		case ACT_ATT3:
			Att3Update(i);
			g_Members[i]._player._act = ACT_IDLE;
			break;
		default:
			// ������ �Ұ�, ������ �Ǽ�
			char err_buff[100];
			sprintf_s(err_buff, sizeof err_buff, "Logic Update�� id : % d�� �̻��� action value : act\n", i);

			if (g_Members[i]._status == PS_VALID)
			{
				g_Members[i]._status = PS_INVALID;
				g_DeleteQueue.push(DeleteJob{ i });
			}
		}
	}
}

void MovingUpdate(int id)
{
	char ip_buffer[20];
	switch (g_Members[id]._player._dir)
	{
	case dfPACKET_MOVE_DIR_LL:
		g_Members[id]._player._x -= 3;
		break;
	case dfPACKET_MOVE_DIR_LU:
		g_Members[id]._player._x -= 3;
		g_Members[id]._player._y -= 2;
		break;
	case dfPACKET_MOVE_DIR_UU:
		g_Members[id]._player._y -= 2;
		break;
	case dfPACKET_MOVE_DIR_RU:
		g_Members[id]._player._x += 3;
		g_Members[id]._player._y -= 2;
		break;
	case dfPACKET_MOVE_DIR_RR:
		g_Members[id]._player._x += 3;
		break;
	case dfPACKET_MOVE_DIR_RD:
		g_Members[id]._player._x += 3;
		g_Members[id]._player._y += 2;
		break;
	case dfPACKET_MOVE_DIR_DD:
		g_Members[id]._player._y += 2;
		break;
	case dfPACKET_MOVE_DIR_LD:
		g_Members[id]._player._x -= 3;
		g_Members[id]._player._y += 2;
		break;
	default:
		// ��� �̻�
		InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
		cout << "[Abnormal] " << ip_buffer << ":" << ntohs(g_Members[id].addr.sin_port) <<
			"MovingDirError : " << g_Members[id]._player._dir << endl << endl;

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
	}

	if (g_Members[id]._player._x < dfRANGE_MOVE_LEFT)
	{
		g_Members[id]._player._x = dfRANGE_MOVE_LEFT;
	}
	else if (g_Members[id]._player._x > dfRANGE_MOVE_RIGHT)
	{
		g_Members[id]._player._x = dfRANGE_MOVE_RIGHT;
	}
	else if (g_Members[id]._player._y < dfRANGE_MOVE_TOP)
	{
		g_Members[id]._player._y = dfRANGE_MOVE_TOP;
	}
	else if (g_Members[id]._player._y > dfRANGE_MOVE_BOTTOM)
	{
		g_Members[id]._player._y = dfRANGE_MOVE_BOTTOM;
	}
}

// ������ ó���ؾ� ������.
void Att1Update(int id)
{
	Proxy proxy;

	// �����̻� �˻�.
	if (g_Members[id]._player._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._player._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "%d���� ���� �̻�. (%d).", id, g_Members[id]._player._dir);
		WriteLog(err_buff,getStackTrace(1).c_str());

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}

	// ���簢���� �� ��� ������.
	int range_x_min;
	int range_y_min;

	// ���簢���� �� �ϴ� ������.
	int range_x_max;
	int range_y_max;


	if (g_Members[id]._player._dir == dfPACKET_MOVE_DIR_LL)
	{
		range_x_max = g_Members[id]._player._x;
		range_x_min = g_Members[id]._player._x - 70;
		range_y_max = g_Members[id]._player._y;
		range_y_min = g_Members[id]._player._y - 100;
	}
	else
	{
		range_x_max = g_Members[id]._player._x + 70;
		range_x_min = g_Members[id]._player._x;
		range_y_max = g_Members[id]._player._y;
		range_y_min = g_Members[id]._player._y - 100;
	}

	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID && i != id)
		{
			if (g_Members[i]._player._x >= range_x_min && g_Members[i]._player._x <= range_x_max &&
				g_Members[i]._player._y >= range_y_min && g_Members[i]._player._y <= range_y_max)
			{
				g_Members[i]._player._hp -= 10;
				if (g_Members[i]._player._hp <= 0)
				{
					cout << i << "�� ���." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// ������ ���
				
				if (g_Members[i]._player._hp > 0)
				{
					proxy.ServerReqDamage(i, SENDMODE_UNI, id, i, g_Members[i]._player._hp);
				}
				proxy.ServerReqDamage(i, SENDMODE_BROAD, id, i, g_Members[i]._player._hp);
				//break;
			}
		}
	}
}

void Att2Update(int id)
{
	Proxy proxy;

	// �����̻� �˻�.
	if (g_Members[id]._player._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._player._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "%d���� ���� �̻�. (%d).", id, g_Members[id]._player._dir);
		WriteLog(err_buff, getStackTrace(1).c_str());

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}

	// ���簢���� �� ��� ������.
	int range_x_min;
	int range_y_min;

	// ���簢���� �� �ϴ� ������.
	int range_x_max;
	int range_y_max;


	if (g_Members[id]._player._dir == dfPACKET_MOVE_DIR_LL)
	{
		range_x_max = g_Members[id]._player._x;
		range_x_min = g_Members[id]._player._x - 70;
		range_y_max = g_Members[id]._player._y;
		range_y_min = g_Members[id]._player._y - 100;
	}
	else
	{
		range_x_max = g_Members[id]._player._x + 70;
		range_x_min = g_Members[id]._player._x;
		range_y_max = g_Members[id]._player._y;
		range_y_min = g_Members[id]._player._y - 100;
	}

	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID && i != id)
		{
			if (g_Members[i]._player._x >= range_x_min && g_Members[i]._player._x <= range_x_max &&
				g_Members[i]._player._y >= range_y_min && g_Members[i]._player._y <= range_y_max)
			{
				g_Members[i]._player._hp -= 10;
				if (g_Members[i]._player._hp <= 0)
				{
					cout << i << "�� ���." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// ������ ���

				if (g_Members[i]._player._hp > 0)
				{
					proxy.ServerReqDamage(i, SENDMODE_UNI, id, i, g_Members[i]._player._hp);
				}
				proxy.ServerReqDamage(i, SENDMODE_BROAD, id, i, g_Members[i]._player._hp);
				//break;
			}
		}
	}
}

void Att3Update(int id)
{
	Proxy proxy;

	// �����̻� �˻�.
	if (g_Members[id]._player._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._player._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "%d���� ���� �̻�. (%d).", id, g_Members[id]._player._dir);
		WriteLog(err_buff, getStackTrace(1).c_str());

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}

	// ���簢���� �� ��� ������.
	int range_x_min;
	int range_y_min;

	// ���簢���� �� �ϴ� ������.
	int range_x_max;
	int range_y_max;


	if (g_Members[id]._player._dir == dfPACKET_MOVE_DIR_LL)
	{
		range_x_max = g_Members[id]._player._x;
		range_x_min = g_Members[id]._player._x - 70;
		range_y_max = g_Members[id]._player._y;
		range_y_min = g_Members[id]._player._y - 100;
	}
	else
	{
		range_x_max = g_Members[id]._player._x + 70;
		range_x_min = g_Members[id]._player._x;
		range_y_max = g_Members[id]._player._y;
		range_y_min = g_Members[id]._player._y - 100;
	}

	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID && i != id)
		{
			if (g_Members[i]._player._x >= range_x_min && g_Members[i]._player._x <= range_x_max &&
				g_Members[i]._player._y >= range_y_min && g_Members[i]._player._y <= range_y_max)
			{
				g_Members[i]._player._hp -= 10;
				if (g_Members[i]._player._hp <= 0)
				{
					cout << i << "�� ���." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// ������ ���

				if (g_Members[i]._player._hp > 0)
				{
					proxy.ServerReqDamage(i, SENDMODE_UNI, id, i, g_Members[i]._player._hp);
				}
				proxy.ServerReqDamage(i, SENDMODE_BROAD, id, i, g_Members[i]._player._hp);
				//break;
			}
		}
	}
}