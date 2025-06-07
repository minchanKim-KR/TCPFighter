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

// PLAYER LIST에서 해당 공간의 상태.
// 사용가능, 연결됨, 삭제예정
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
	// list 공간 상태
	PS_Status _status;
	// 패킷 작업 지점.
	BOOL _is_payload; // payload 작업중
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

// PLAYER LIST에서 삭제할 멤버 ID
struct DeleteJob
{
	int _id;
};

// DATA
Player g_Members[TOTAL_PLAYER];
queue<DeleteJob> g_DeleteQueue;
int g_numMembers; // 접속자 수

//FPS (안쓸듯)
//DWORD old_for_1s;
//DWORD old;
//DWORD deltaTime;
//int fps;
//int fps_for_print;

//PACKET
// 해당 프레임에 처리한 패킷 수
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
		// 유효하지 않은 소켓
		// 이미 바인딩 된 소켓
		// 이미 사용중인 포트(동일 ip + port 조합은 한 소켓만 사용가능)

		cout << "Error : bind()" << endl;
		ret_bind = GetLastError();
		cout << "ErrCode : " << ret_bind << endl;
		return;
	}

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		// 유효하지 않은 소켓

		wcout << L"Error : listen()" << endl;
		ret_listen = GetLastError();
		wcout << L"ErrCode : " << ret_listen << endl;
		return;
	}

	//////////////////////////////////////////////////////
	// Non Blocking Socket으로 전환
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

		//50 fps 고정
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
			// LISTEN을 닫은 경우
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
		// 밀고있지 않으므로 소켓 번호가 겹칠수있음.
		if (g_Members[i]._status == PS_VALID && FD_ISSET(g_Members[i].sock, &rset))
		{
			char temp_buf[MAX_RINGBUF_SIZE];
			int recv_cap = g_Members[i]._recvBuf.GetCapacity();
			//////////////////////////////////////////////////////
			// TCP 버퍼 -> Temp (recv_cap 0 들어가도 문제없음)
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
					// 로깅
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "Recv()에서 알 수 없는 에러.\n ERRORCODE : %d", GetLastError());
					cout << "이상에러" << endl;
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
					// 로깅
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Enqueue() 동작 에러.");
					WriteLog(err_buff);

					return false;
				}

				//////////////////////////////////////////////////////
				// 수신버퍼에 있는 패킷 처리
				//////////////////////////////////////////////////////
				char ip_buffer[20];
				while (1)
				{
					Header h;
					int num_deq;
					
					//////////////////////////////////////////////////////
					// 헤더
					//////////////////////////////////////////////////////
					if (g_Members[i]._is_payload == false)
					{
						// 헤더작업
						if (g_Members[i]._recvBuf.GetDataSize() < sizeof Header)
							break;

						int num_deq;
						num_deq = g_Members[i]._recvBuf.Dequeue((char*)&h, sizeof Header);
						if (num_deq != sizeof Header)
						{
							// 링버퍼 문제 로깅
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
							WriteLog(err_buff);

							return false;
						}

						if (h.byCode != 0x89 || h.bySize > MAX_PAYLOAD_SIZE)
						{
							// 헤더 이상
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
							// 링버퍼 문제 로깅
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
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
							// 링버퍼 문제 로깅
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
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
							// 링버퍼 문제 로깅
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
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
							// 링버퍼 문제 로깅
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
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
							// 링버퍼 문제 로깅
							char err_buff[100];
							sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
							WriteLog(err_buff);

							return false;
						}
						Attack3(wset, i, &req_att3);

						g_Members[i]._is_payload = false;
						break;
					default:
						// 패킷 이상
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

	// 주의 : select는 아무것도 안넣으면 
	// (모든 셋의 COUNT가 0이면) ERROR 10022 매개변수문제
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
		// 10022 매개변수 문제, rset, wset count가 전부 0인채로 실행되면 error
		ret_select = GetLastError();
		printf_s("send select ErrorCode : %d.\n", ret_select);
		return false;
	}

	//////////////////////////////////////////////////////
	// SEND
	//////////////////////////////////////////////////////
	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		// 밀고있지 않으므로 소켓 번호가 겹칠수있음.
		if (g_Members[i]._status == PS_VALID && FD_ISSET(g_Members[i].sock, &wset))
		{
			int retval;
			char send_buf[MAX_RINGBUF_SIZE];
			int num_deq = g_Members[i]._sendBuf.GetDataSize();
			if (g_Members[i]._sendBuf.Dequeue(send_buf, num_deq) != num_deq)
			{
				// 로깅
				char err_buff[100];
				sprintf_s(err_buff, sizeof err_buff, "SendBuf.Enqueue() 동작 에러.");
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
				// SEND시도, 연결 끊어지면 CLOSESOCKET QUEUEING
			}
		}
	}
	return true;
}

// 플레이어 참여에 대한 처리
void PlayerIsJoined(FD_SET& wset, SOCKET& clientSock, SOCKADDR_IN& clientAddr)
{
	char packet[sizeof(Header) + sizeof(ServerRequestCreatePlayer)];
	char ip_buffer[20];
	int id = SetId();
	if (id == -1)
	{
		// 연결 해제
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
		"님의 접속.(연결중 : " << g_numMembers << ")" << endl << endl;

	// 1. client id 할당 및 캐릭터 생성
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

	// 2. 다른 참여자에게 해당플레이어 캐릭터 생성
	header->byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	SendBroadcast(wset, id, packet, sizeof(Header) + sizeof(ServerRequestCreatePlayer));

	// 여기서 플레이어 상태까지 보내야함.
	// 3. 접속자에게 내부 인원들에 대한 위치, 상태 전달
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

			// 애니메이션 동기화는 신경쓰지 않는다.

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

// 자리가 있는지 검사하고, ID해결
int SetId()
{
	// HACK : 위치할 배열의 인덱스를 ID로 결정.
	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		if (g_Members[i]._status == PS_EMPTY)
		{
			return i;
		}
	}

	// 자리가 없으면 연결 해제.
	return -1;
}

// ID에 해당하는 CLIENT에 패킷 전송.
void SendUnicast(FD_SET& wset, int id, char* packet, int len)
{
	int enq_num;
	enq_num = g_Members[id]._sendBuf.Enqueue(packet, len);
	if (enq_num == 0)
	{
		// send 링버퍼 꽉차면 연결 해제
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
	}
	else if (enq_num != len)
	{
		// 링버퍼 고장
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "SendBuf.Enqueue() 동작 에러.");
		WriteLog(err_buff);
	}
}

// ID에 해당하는 CLINET를 제외한 모두에게 패킷 전송.
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
				// send 링버퍼 꽉차면 연결 해제
				if (g_Members[i]._status == PS_VALID)
				{
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ i });
				}
			}
			else if (enq_num != len)
			{
				// 링버퍼 고장
				char err_buff[100];
				sprintf_s(err_buff, sizeof err_buff, "SendBuf.Enqueue() 동작 에러.");
				WriteLog(err_buff);
			}
		}
	}
}
void DeleteSockets(FD_SET& wset)
{
	// 연결하자마자 끊기면
	// connect에서 한번, read에서 rst로 또한번
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
			"님의 접속 종료. (연결중 : " << g_numMembers << ")" << endl << endl;
	
		// 죽은 캐릭터 제거
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
	// 1. 움직임 시작.
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
	// 0. 이상한 방향이면 접속차단
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "정지상태에서 id : %d님의 방향 이상. (%d).", id, packet->_dir);
		WriteLog(err_buff);

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}
	// 1. 움직임 정지.
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
	// 0. 방향검사
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}
	// 1. 공격요청
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
	// 0. 방향검사
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}
	// 1. 공격요청
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
	// 0. 방향검사
	if (packet->_dir != dfPACKET_MOVE_DIR_LL && packet->_dir != dfPACKET_MOVE_DIR_RR)
	{
		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}

	// 1. 공격요청
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
			// 임시. 반드시 수정.
			Att1Update(wset, i);
			g_Members[i]._act = ACT_IDLE;
			break;
		case ACT_ATT3:
			// 임시. 반드시 수정.
			Att1Update(wset, i);
			g_Members[i]._act = ACT_IDLE;
			break;
		default:
			// 로직상 불가, 서버의 실수
			char err_buff[100];
			sprintf_s(err_buff, sizeof err_buff, "Logic Update중 id : % d의 이상한 action value : act\n", i);

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
		// 헤더 이상
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

// 프레임 처리해야 판정됨.
void Att1Update(FD_SET& wset, int id)
{
	// 방향이상 검사.
	if (g_Members[id]._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "att1 id : %d님의 방향 이상. (%d).", id, g_Members[id]._dir);
		WriteLog(err_buff);

		if (g_Members[id]._status == PS_VALID)
		{
			g_Members[id]._status = PS_INVALID;
			g_DeleteQueue.push(DeleteJob{ id });
		}
		return;
	}

	// 직사각형의 좌 상단 꼭짓점.
	int range_x_min;
	int range_y_min;

	// 직사각형의 우 하단 꼭짓점.
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
					cout << i << "님 사망." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// 데미지 계산
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

