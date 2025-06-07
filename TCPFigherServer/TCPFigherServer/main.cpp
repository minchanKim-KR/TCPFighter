#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <Windows.h>
#include <queue>

#include "WriteLogTXT.h"
#include "RingBuffer.h"
#include "PacketDefine.h"
#include "NetworkPacket.h"

using namespace std;

#define SERVER_PORT 5000

#define TOTAL_PLAYER 60
#define HEADER_SIZE 3

// PLAYER LIST���� �ش� ������ ����.
// ��밡��, �����, ��������
enum PS_Status
{
	// player status
	PS_EMPTY = 0, PS_VALID, PS_INVALID,
};
enum Player_Action
{
	ACT_IDLE, ACT_MOVESTART, ACT_MOVING, ACT_ATT1, ACT_ATT2, ACT_ATT3
};

struct Player
{
	// list ���� ����
	PS_Status _status;
	// ��Ŷ �۾� ����.
	BOOL _is_payload; // payload �۾���
	int _h_type;

	
	MyRingBuffer _recvBuf;
	MyRingBuffer _sendBuf;

	SOCKET sock;
	SOCKADDR_IN addr;
	ULONGLONG _tStamp;
	int _id;

	Player_Action _act;
	BYTE _dir;
	BYTE _hp;
	int _y;
	int _x;
};

// PLAYER LIST���� ������ ��� ID
struct DeleteJob
{
	int _id;
};

// DATA
Player g_Members[TOTAL_PLAYER];
queue<DeleteJob> g_DeleteQueue;
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
void SendUnicast(FD_SET& wset, int id, char* packet, int len);
void SendBroadcast(FD_SET& wset, int id, char* packet, int len);
void DeleteSockets(FD_SET& wset);

void MoveStart(FD_SET& wset, int id, ClientRequestMoveStart* packet);
void MoveStop(FD_SET& wset, int id, ClientRequestMoveStop* packet);
void Attack1(FD_SET& wset, int id, ClientRequestAtt1* packet);
void Attack2(FD_SET& wset, int id, ClientRequestAtt2* packet);
void Attack3(FD_SET& wset, int id, ClientRequestAtt3* packet);

void UpdateLogic(FD_SET& wset);
void MovingUpdate(int id);
void Att1Update(FD_SET& wset, int id);

int SetId();
void PlayerIsJoined(FD_SET& wset, SOCKET& clientSock, SOCKADDR_IN& clientAddr);

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

		PlayerIsJoined(wset, client_sock, clientaddr);
	}

	//////////////////////////////////////////////////////
	// RECV
	//////////////////////////////////////////////////////
	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		// �а����� �����Ƿ� ���� ��ȣ�� ��ĥ������.
		if (g_Members[i]._status == PS_VALID && FD_ISSET(g_Members[i].sock, &rset))
		{
			char temp_buf[MAX_RINGBUF_SIZE];
			int recv_cap = g_Members[i]._recvBuf.GetCapacity();
			//////////////////////////////////////////////////////
			// TCP ���� -> Temp (recv_cap 0 ���� ��������)
			//////////////////////////////////////////////////////
			retval = recv(g_Members[i].sock, temp_buf, recv_cap, 0);
			g_num_packet++;
			if (retval == SOCKET_ERROR)
			{
				if (GetLastError() == 10054)
				{
					// RST (disconnect)
				}
				else
				{
					// �α�
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "Recv()���� �� �� ���� ����.\n ERRORCODE : %d", GetLastError());
					cout << "�̻󿡷�" << endl;
					WriteLog(err_buff, "Recv Error");
				}
				g_Members[i]._status = PS_INVALID;
				g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
			}
			else if (retval == 0)
			{
				// FIN (disconnect)
				g_Members[i]._status = PS_INVALID;
				g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
			}
			else
			{
				//////////////////////////////////////////////////////
				// Temp -> RecvRingBuf
				//////////////////////////////////////////////////////
				int num_enq = g_Members[i]._recvBuf.Enqueue(temp_buf, retval);
				if (num_enq != retval)
				{
					// �α�
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Enqueue() ���� ����.");
					WriteLog(err_buff);

					return false;
				}

				//////////////////////////////////////////////////////
				// ���Ź��ۿ� �ִ� ��Ŷ ó��
				//////////////////////////////////////////////////////
				char ip_buffer[20];
				while (1)
				{
					Header h;
					int num_deq;
					
					//////////////////////////////////////////////////////
					// ���
					//////////////////////////////////////////////////////
					if (g_Members[i]._is_payload == false)
					{
						// ����۾�
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof Header)
							break;

						int num_deq;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&h, sizeof Header);
						if (num_deq != sizeof Header)
						{
							// ������ ���� �α�
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
							WriteLog(err_buff);

							return false;
						}

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

						g_Members[i]._is_payload = true;
						g_Members[i]._h_type = h.byType;
					}
					
					//////////////////////////////////////////////////////
					// PAYLOAD
					//////////////////////////////////////////////////////

					BYTE type = g_Members[i]._h_type;
					bool is_waiting_payload = false;
					char payload[MAX_PAYLOAD_SIZE];

					switch (type)
					{
					case dfPACKET_CS_MOVE_START:
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof ClientRequestMoveStart)
						{
							is_waiting_payload = true;
							break;
						}
						ClientRequestMoveStart req_mstrt;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&req_mstrt, sizeof ClientRequestMoveStart);
						if (num_deq != sizeof ClientRequestMoveStart)
						{
							// ������ ���� �α�
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
							WriteLog(err_buff);

							return false;
						}
						MoveStart(wset, i, &req_mstrt);

						g_Members[i]._is_payload = false;
						break;
					case dfPACKET_CS_MOVE_STOP:
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof ClientRequestMoveStop)
						{
							is_waiting_payload = true;
							break;
						}
						ClientRequestMoveStop req_mstp;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&req_mstp, sizeof ClientRequestMoveStop);
						if (num_deq != sizeof ClientRequestMoveStop)
						{
							// ������ ���� �α�
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
							WriteLog(err_buff);

							return false;
						}
						MoveStop(wset, i, &req_mstp);

						g_Members[i]._is_payload = false;
						break;
					case dfPACKET_CS_ATTACK1:
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof ClientRequestAtt1)
						{
							is_waiting_payload = true;
							break;
						}
						ClientRequestAtt1 req_att1;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&req_att1, sizeof ClientRequestAtt1);
						if (num_deq != sizeof ClientRequestAtt1)
						{
							// ������ ���� �α�
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
							WriteLog(err_buff);

							return false;
						}
						Attack1(wset, i, &req_att1);

						g_Members[i]._is_payload = false;
						break;
					case dfPACKET_CS_ATTACK2:
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof ClientRequestAtt2)
						{
							is_waiting_payload = true;
							break;
						}
						ClientRequestAtt2 req_att2;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&req_att2, sizeof ClientRequestAtt2);
						if (num_deq != sizeof ClientRequestAtt2)
						{
							// ������ ���� �α�
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
							WriteLog(err_buff);

							return false;
						}
						Attack2(wset, i, &req_att2);

						g_Members[i]._is_payload = false;
						break;
					case dfPACKET_CS_ATTACK3:
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof ClientRequestAtt3)
						{
							is_waiting_payload = true;
							break;
						}
						ClientRequestAtt3 req_att3;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&req_att3, sizeof ClientRequestAtt3);
						if (num_deq != sizeof ClientRequestAtt3)
						{
							// ������ ���� �α�
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() ���� ����.");
							WriteLog(err_buff);

							return false;
						}
						Attack3(wset, i, &req_att3);

						g_Members[i]._is_payload = false;
						break;
					default:
						// ��Ŷ �̻�
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ g_Members[i]._id });
						break;
					}

					if (is_waiting_payload == true)
					{
						break;
					}
					else
					{
						g_Members[i]._is_payload = false;
					}

				}
			}
		}
	}

	//////////////////////////////////////////////////////
	// Logic
	////////////////////////////////////////////////////// 
	UpdateLogic(wset);

	//////////////////////////////////////////////////////
	// Delete
	//////////////////////////////////////////////////////
	DeleteSockets(wset);
	
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
			char send_buf[MAX_RINGBUF_SIZE];
			int num_deq = g_Members[i]._sendBuf.GetDataSize();
			if (g_Members[i]._sendBuf.Dequeue(send_buf, num_deq) != num_deq)
			{
				// �α�
				char err_buff[100];
				sprintf_s(err_buff, sizeof err_buff, "SendBuf.Enqueue() ���� ����.");
				WriteLog(err_buff);

				return false;
			}

			retval = send(g_Members[i].sock, send_buf, num_deq, 0);
			if (retval == SOCKET_ERROR)
			{
				int send_error = GetLastError();
				if (g_Members[i]._status == PS_VALID)
				{
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ i });
				}
				// SEND�õ�, ���� �������� CLOSESOCKET QUEUEING
			}
		}
	}
	return true;
}

// �÷��̾� ������ ���� ó��
void PlayerIsJoined(FD_SET& wset, SOCKET& clientSock, SOCKADDR_IN& clientAddr)
{
	char packet[sizeof(Header) + sizeof(ServerRequestCreatePlayer)];
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

	g_Members[id]._act = ACT_IDLE;
	g_Members[id]._dir = dfPACKET_MOVE_DIR_LL;
	g_Members[id]._x = (rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT)) + dfRANGE_MOVE_LEFT;
	g_Members[id]._y = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP)) + dfRANGE_MOVE_TOP;;
	g_Members[id]._hp = 100;
	g_Members[id]._is_payload = false;
	g_numMembers++;

	InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
	cout << "[CON] " << ip_buffer << ":" << ntohs(g_Members[id].addr.sin_port) <<
		"���� ����.(������ : " << g_numMembers << ")" << endl << endl;

	// 1. client id �Ҵ� �� ĳ���� ����
	ZeroMemory(packet, sizeof(Header) + sizeof(ServerRequestCreatePlayer));
	Header* header = (Header*)packet;
	header->byCode = 0x89;
	header->bySize = sizeof(ServerRequestCreatePlayer);
	header->byType = dfPACKET_SC_CREATE_MY_CHARACTER;
	ServerRequestCreatePlayer* payload = 
		(ServerRequestCreatePlayer*)(packet + sizeof(Header));
	payload->_id = id;
	payload->_dir = g_Members[id]._dir;
	payload->_hp = g_Members[id]._hp;
	payload->_x = g_Members[id]._x;
	payload->_y = g_Members[id]._y;
	SendUnicast(wset, id, packet, sizeof(Header) + sizeof(ServerRequestCreatePlayer));

	// 2. �ٸ� �����ڿ��� �ش��÷��̾� ĳ���� ����
	header->byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	SendBroadcast(wset, id, packet, sizeof(Header) + sizeof(ServerRequestCreatePlayer));

	// ���⼭ �÷��̾� ���±��� ��������.
	// 3. �����ڿ��� ���� �ο��鿡 ���� ��ġ, ���� ����
	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status != PS_EMPTY && g_Members[i]._id != id)
		{
			ZeroMemory(packet, sizeof(Header) + sizeof(ServerRequestCreatePlayer));
			header = (Header*)packet;
			header->byCode = 0x89;
			header->bySize = sizeof(ServerRequestCreatePlayer);
			header->byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
			
			payload = (ServerRequestCreatePlayer*)(packet + sizeof(Header));
			payload->_id = g_Members[i]._id;
			payload->_dir = g_Members[i]._dir;
			payload->_hp = g_Members[i]._hp;
			payload->_x = g_Members[i]._x;
			payload->_y = g_Members[i]._y;
			SendUnicast(wset, id, packet, sizeof(Header) + sizeof(ServerRequestCreatePlayer));

			// �ִϸ��̼� ����ȭ�� �Ű澲�� �ʴ´�.

			if (g_Members[i]._act == ACT_MOVING)
			{
				char packet_move_start[sizeof(Header) + sizeof(ServerRequestMoveStart)];
				ZeroMemory(packet_move_start, sizeof(Header) + sizeof(ServerRequestMoveStart));
				header = (Header*)packet_move_start;
				header->byCode = 0x89;
				header->bySize = sizeof(ServerRequestMoveStart);
				header->byType = dfPACKET_SC_MOVE_START;

				ServerRequestMoveStart* payload_move_start;
				payload_move_start = (ServerRequestMoveStart*)(packet_move_start + sizeof(Header));
				payload_move_start->_id = g_Members[i]._id;
				payload_move_start->_dir = g_Members[i]._dir;
				payload_move_start->_x = g_Members[i]._x;
				payload_move_start->_y = g_Members[i]._y;
				SendUnicast(wset, id, packet_move_start, sizeof(Header) + sizeof(ServerRequestMoveStart));
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

// ID�� �ش��ϴ� CLIENT�� ��Ŷ ����.
void SendUnicast(FD_SET& wset, int id, char* packet, int len)
{
	int enq_num;
	enq_num = g_Members[id]._sendBuf.Enqueue(packet, len);
	if (enq_num == 0)
	{
		// send ������ ������ ���� ����
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
	}
	else if (enq_num != len)
	{
		// ������ ����
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "SendBuf.Enqueue() ���� ����.");
		WriteLog(err_buff);
	}
}

// ID�� �ش��ϴ� CLINET�� ������ ��ο��� ��Ŷ ����.
void SendBroadcast(FD_SET& wset, int id, char* packet, int len)
{
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
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ i });
				}
			}
			else if (enq_num != len)
			{
				// ������ ����
				char err_buff[100];
				sprintf_s(err_buff, sizeof err_buff, "SendBuf.Enqueue() ���� ����.");
				WriteLog(err_buff);
			}
		}
	}
}
void DeleteSockets(FD_SET& wset)
{
	// �������ڸ��� �����
	// connect���� �ѹ�, read���� rst�� ���ѹ�
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
		char req_packet[sizeof(Header) + sizeof(DeletePlayer)];
		ZeroMemory(req_packet, sizeof(Header) + sizeof(DeletePlayer));
		Header* h = (Header*)req_packet;
		h->byCode = 0x89;
		h->bySize = sizeof(dfPACKET_SC_DELETE_CHARACTER);
		h->byType = dfPACKET_SC_DELETE_CHARACTER;
		DeletePlayer* payload = (DeletePlayer*)(req_packet + sizeof(Header));
		payload->_id = id;

		SendUnicast(wset, id, req_packet, sizeof(Header) + sizeof(DeletePlayer));
		SendBroadcast(wset, id, req_packet, sizeof(Header) + sizeof(DeletePlayer));
		break;
	}
}

void MoveStart(FD_SET& wset, int id, ClientRequestMoveStart* packet)
{
	// 1. ������ ����.
	g_Members[id]._act = ACT_MOVESTART;
	g_Members[id]._dir = packet->_dir;
	g_Members[id]._x = packet->_x;
	g_Members[id]._y = packet->_y;

	// 2. broadcast
	char req_packet[sizeof(Header) + sizeof(ServerRequestMoveStart)];
	ZeroMemory(req_packet, sizeof(Header) + sizeof(ServerRequestMoveStart));
	Header* h = (Header*)req_packet;
	h->byCode = 0x89;
	h->bySize = sizeof(ServerRequestMoveStart);
	h->byType = dfPACKET_SC_MOVE_START;
	ServerRequestMoveStart* payload = (ServerRequestMoveStart*)(req_packet + sizeof(Header));
	payload->_dir = packet->_dir;
	payload->_id = id;
	payload->_x = packet->_x;
	payload->_y = packet->_y;
	SendBroadcast(wset, id, req_packet, sizeof(Header) + sizeof(ServerRequestMoveStart));
}

void MoveStop(FD_SET& wset, int id, ClientRequestMoveStop* packet)
{
	// 0. �̻��� �����̸� ��������
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "�������¿��� id : %d���� ���� �̻�. (%d).", id, packet->_dir);
		WriteLog(err_buff);

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}
	// 1. ������ ����.
	g_Members[id]._act = ACT_IDLE;
	g_Members[id]._dir = packet->_dir;
	g_Members[id]._x = packet->_x;
	g_Members[id]._y = packet->_y;

	// 2. broadcast
	char req_packet[sizeof(Header) + sizeof(ServerRequestMoveStart)];
	ZeroMemory(req_packet, sizeof(Header) + sizeof(ServerRequestMoveStart));
	Header* h = (Header*)req_packet;
	h->byCode = 0x89;
	h->bySize = sizeof(ServerRequestMoveStop);
	h->byType = dfPACKET_SC_MOVE_STOP;

	ServerRequestMoveStop* payload = (ServerRequestMoveStop*)(req_packet + sizeof(Header));
	payload->_dir = packet->_dir;
	payload->_id = id;
	payload->_x = packet->_x;
	payload->_y = packet->_y;
	SendBroadcast(wset, id, req_packet, sizeof(Header) + sizeof(ServerRequestMoveStop));
}

void Attack1(FD_SET& wset, int id, ClientRequestAtt1* packet)
{
	// 0. ����˻�
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}
	// 1. ���ݿ�û
	g_Members[id]._act = ACT_ATT1;
	g_Members[id]._dir = packet->_dir;
	g_Members[id]._x = packet->_x;
	g_Members[id]._y = packet->_y;

	// 2. broadcast
	char req_packet[sizeof(Header) + sizeof(ServerRequestAtt1)];
	ZeroMemory(req_packet, sizeof(Header) + sizeof(ServerRequestAtt1));
	Header* h = (Header*)req_packet;
	h->byCode = 0x89;
	h->bySize = sizeof(ServerRequestAtt1);
	h->byType = dfPACKET_SC_ATTACK1;

	ServerRequestAtt1* payload = (ServerRequestAtt1*)(req_packet + sizeof(Header));
	payload->_dir = packet->_dir;
	payload->_id = id;
	payload->_x = packet->_x;
	payload->_y = packet->_y;
	SendBroadcast(wset, id, req_packet, sizeof(Header) + sizeof(ServerRequestAtt1));
}

void Attack2(FD_SET& wset, int id, ClientRequestAtt2* packet)
{
	// 0. ����˻�
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}
	// 1. ���ݿ�û
	g_Members[id]._act = ACT_ATT2;
	g_Members[id]._dir = packet->_dir;
	g_Members[id]._x = packet->_x;
	g_Members[id]._y = packet->_y;

	// 2. broadcast
	char req_packet[sizeof(Header) + sizeof(ServerRequestAtt2)];
	ZeroMemory(req_packet, sizeof(Header) + sizeof(ServerRequestAtt2));
	Header* h = (Header*)req_packet;
	h->byCode = 0x89;
	h->bySize = sizeof(ServerRequestAtt2);
	h->byType = dfPACKET_SC_ATTACK2;

	ServerRequestAtt2* payload = (ServerRequestAtt2*)(req_packet + sizeof(Header));
	payload->_dir = packet->_dir;
	payload->_id = id;
	payload->_x = packet->_x;
	payload->_y = packet->_y;
	SendBroadcast(wset, id, req_packet, sizeof(Header) + sizeof(ServerRequestAtt2));
}

void Attack3(FD_SET& wset, int id, ClientRequestAtt3* packet)
{
	// 0. ����˻�
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}

	// 1. ���ݿ�û
	g_Members[id]._act = ACT_ATT3;
	g_Members[id]._dir = packet->_dir;
	g_Members[id]._x = packet->_x;
	g_Members[id]._y = packet->_y;

	// 2. broadcast
	char req_packet[sizeof(Header) + sizeof(ServerRequestAtt3)];
	ZeroMemory(req_packet, sizeof(Header) + sizeof(ServerRequestAtt3));
	Header* h = (Header*)req_packet;
	h->byCode = 0x89;
	h->bySize = sizeof(ServerRequestAtt3);
	h->byType = dfPACKET_SC_ATTACK3;

	ServerRequestAtt3* payload = (ServerRequestAtt3*)(req_packet + sizeof(Header));
	payload->_dir = packet->_dir;
	payload->_id = id;
	payload->_x = packet->_x;
	payload->_y = packet->_y;
	SendBroadcast(wset, id, req_packet, sizeof(Header) + sizeof(ServerRequestAtt3));
}

void UpdateLogic(FD_SET& wset)
{
	for (int i = 0; i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status != PS_VALID)
			continue;

		Player_Action act = g_Members[i]._act;
		switch (act)
		{
		case ACT_MOVESTART:
			g_Members[i]._act = ACT_MOVING;
			break;
		case ACT_MOVING:
			MovingUpdate(i);
			break;
		case ACT_IDLE:
			break;
		case ACT_ATT1:
			Att1Update(wset, i);
			g_Members[i]._act = ACT_IDLE;
			break;
		case ACT_ATT2:
			// �ӽ�. �ݵ�� ����.
			Att1Update(wset, i);
			g_Members[i]._act = ACT_IDLE;
			break;
		case ACT_ATT3:
			// �ӽ�. �ݵ�� ����.
			Att1Update(wset, i);
			g_Members[i]._act = ACT_IDLE;
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
	switch (g_Members[id]._dir)
	{
	case dfPACKET_MOVE_DIR_LL:
		g_Members[id]._x -= 3;
		break;
	case dfPACKET_MOVE_DIR_LU:
		g_Members[id]._x -= 3;
		g_Members[id]._y -= 2;
		break;
	case dfPACKET_MOVE_DIR_UU:
		g_Members[id]._y -= 2;
		break;
	case dfPACKET_MOVE_DIR_RU:
		g_Members[id]._x += 3;
		g_Members[id]._y -= 2;
		break;
	case dfPACKET_MOVE_DIR_RR:
		g_Members[id]._x += 3;
		break;
	case dfPACKET_MOVE_DIR_RD:
		g_Members[id]._x += 3;
		g_Members[id]._y += 2;
		break;
	case dfPACKET_MOVE_DIR_DD:
		g_Members[id]._y += 2;
		break;
	case dfPACKET_MOVE_DIR_LD:
		g_Members[id]._x -= 3;
		g_Members[id]._y += 2;
		break;
	default:
		// ��� �̻�
		InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
		cout << "[Abnormal] " << ip_buffer << ":" << ntohs(g_Members[id].addr.sin_port) <<
			"MovingDirError : " << g_Members[id]._dir << endl << endl;

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
	}

	if (g_Members[id]._x < dfRANGE_MOVE_LEFT)
	{
		g_Members[id]._x = dfRANGE_MOVE_LEFT;
	}
	else if (g_Members[id]._x > dfRANGE_MOVE_RIGHT)
	{
		g_Members[id]._x = dfRANGE_MOVE_RIGHT;
	}
	else if (g_Members[id]._y < dfRANGE_MOVE_TOP)
	{
		g_Members[id]._y = dfRANGE_MOVE_TOP;
	}
	else if (g_Members[id]._y > dfRANGE_MOVE_BOTTOM)
	{
		g_Members[id]._y = dfRANGE_MOVE_BOTTOM;
	}
}

// ������ ó���ؾ� ������.
void Att1Update(FD_SET& wset, int id)
{
	// �����̻� �˻�.
	if (g_Members[id]._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "att1 id : %d���� ���� �̻�. (%d).", id, g_Members[id]._dir);
		WriteLog(err_buff);

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


	if (g_Members[id]._dir == dfPACKET_MOVE_DIR_LL)
	{
		range_x_max = g_Members[id]._x;
		range_x_min = g_Members[id]._x - 70;
		range_y_max = g_Members[id]._y;
		range_y_min = g_Members[id]._y - 100;
	}
	else
	{
		range_x_max = g_Members[id]._x + 70;
		range_x_min = g_Members[id]._x;
		range_y_max = g_Members[id]._y;
		range_y_min = g_Members[id]._y - 100;
	}

	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_VALID && i != id)
		{
			if (g_Members[i]._x >= range_x_min && g_Members[i]._x <= range_x_max &&
				g_Members[i]._y >= range_y_min && g_Members[i]._y <= range_y_max)
			{
				g_Members[i]._hp -= 10;
				if (g_Members[i]._hp <= 0)
				{
					cout << i << "�� ���." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// ������ ���
				char req_packet[sizeof(Header) + sizeof(ServerRequestDamageStep)];
				ZeroMemory(req_packet, sizeof(Header) + sizeof(ServerRequestDamageStep));
				Header* h = (Header*)req_packet;
				h->byCode = 0x89;
				h->bySize = sizeof(ServerRequestDamageStep);
				h->byType = dfPACKET_SC_DAMAGE;
				ServerRequestDamageStep* payload = (ServerRequestDamageStep*)(req_packet + sizeof(Header));
				payload->_id_attacker = id;
				payload->_id_defender = i;
				payload->_hp_defender = g_Members[i]._hp;

				if (g_Members[i]._hp > 0)
				{
					SendUnicast(wset, i, req_packet, sizeof(Header) + sizeof(ServerRequestDamageStep));
				}

				SendBroadcast(wset, i, req_packet, sizeof(Header) + sizeof(ServerRequestDamageStep));
				//break;
			}
		}
	}
}

