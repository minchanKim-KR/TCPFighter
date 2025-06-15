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

// Select Model은 기본적으로 set당 최대 64개의 소켓을 검사 가능.

#define SERVER_PORT 5000

// Content에 접속 가능한 유저 수
#define TOTAL_PLAYER 60
#define HEADER_SIZE 3

// DATA
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

	// 0.0.0.0:5000 에 대해 Listen 
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

		//50 fps 제한
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

		PlayerIsJoined(client_sock, clientaddr);
	}

	//////////////////////////////////////////////////////
	// RECV
	//////////////////////////////////////////////////////
	for (i = 0;i < TOTAL_PLAYER;i++)
	{
		// 밀고있지 않으므로 소켓 번호가 겹칠수있음.
		if (g_Members[i]._status == PS_VALID && FD_ISSET(g_Members[i].sock, &rset))
		{
			int now_cap = g_Members[i]._recvBuf.GetCapacity();
			int num_direct_enq = g_Members[i]._recvBuf.DirectEnqueueSize();
			////////////////////////////////////////////////////////////////
			// 1. 남는 공간이 없으면 Recv하지 않음.
			////////////////////////////////////////////////////////////////
			if (now_cap < 1)
			{
				continue;
			}
			////////////////////////////////////////////////////////////////
			// 2. Select된 소켓이므로 Recv시도.
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
				// 예상하지 못한 오류
				else
				{
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "Recv()에서 알 수 없는 에러.\n ERRORCODE : %d", GetLastError());
					cout << "이상에러" << endl;
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
			// 정상 동작
			else
			{
				g_Members[i]._recvBuf.MoveTail(retval);
				g_Members[i]._recvBuf._dataSize += retval;
			}
			////////////////////////////////////////////////////////////////////////////
			// 3. ringbuffer를 direct 끝까지 채움, 링버퍼가 꽉 차지 않음.
			////////////////////////////////////////////////////////////////////////////
			if (retval == num_direct_enq && !g_Members[i]._recvBuf.IsFull())
			{
				//cout << "recv버퍼 한바퀴 돌았음. " << endl;
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
					// recv할 내용이 없음.
				}
				else if (retval == SOCKET_ERROR)
				{
					// RST (disconnect)
					if (GetLastError() == 10054)
					{
						
					}
					// 예상하지 못한 오류
					else
					{
						char err_buff[100];
						sprintf_s(err_buff, sizeof err_buff, "Recv()에서 알 수 없는 에러.\n ERRORCODE : %d", GetLastError());
						cout << "이상에러" << endl;
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
				// 정상 동작
				else
				{
					g_Members[i]._recvBuf.MoveTail(retval);
					g_Members[i]._recvBuf._dataSize += retval;
				}

				cout << "추가 데이터 삽입 " << endl;
				cout << "retval : " << retval << endl;
				cout << "Head : " << g_Members[i]._recvBuf._head << endl;
				cout << "Tail : " << g_Members[i]._recvBuf._tail << endl;
				cout << endl;
			}

			g_num_packet++;
			
			//////////////////////////////////////////////////////
			// 수신버퍼에 있는 패킷 처리
			//////////////////////////////////////////////////////
			char ip_buffer[20];
			while (1)
			{
				Header h;
				int num_deq;

				//////////////////////////////////////////////////////
				// Header 검사
				//////////////////////////////////////////////////////
				// 헤더작업
				if (g_Members[i]._recvBuf.GetDataSize() < sizeof Header)
					break;

				num_deq = g_Members[i]._recvBuf.Peek((char*)&h, sizeof Header);
				if (num_deq != sizeof Header)
				{
					// 링버퍼 문제 로깅
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "RecvBuf.Dequeue() 동작 에러.");
					WriteLog(err_buff);

					return false;
				}

				// 적합한 헤더인가?
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

				// payload가 전부 버퍼에 있나?
				if (g_Members[i]._recvBuf.GetDataSize() < sizeof Header + h.bySize)
					break;

				//////////////////////////////////////////////////////
				// PAYLOAD
				//////////////////////////////////////////////////////

				// PacketProcedure(Ringbuf& rb);

				// 1. Header와 payload를 나눈다.
				// Header, SerializingBuffer.PutData <- Ringbuf.Dequeue

				// 2. 매개변수 (char*, list<>&는 동적할당?) -> 일단 동적할당.
				// 3. 원시타입은 그냥 sb에서 빼면됨.
				// 4. 동일한 시그니쳐를 호출시킴
				// 5. 해당 시그니쳐를 구현한 실제 iStub recv_stb = new Stub에 대한 stb.Mk_packet(...);을 호출
						
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
			
			// 1. send를 시도
			retval = send(g_Members[i].sock
				, g_Members[i]._sendBuf.GetHeadPtr()
				, g_Members[i]._sendBuf.DirectDequeueSize()
				, 0);			
			if (retval == SOCKET_ERROR)
			{
				//select되었기 때문에 버퍼가 꽉 차 wouldblock 걸릴 일이 없다.

				int send_error = GetLastError();

				// RST (disconnect) WSAECONNRESET (10054)
				if (send_error == WSAECONNRESET)
				{
					
				}
				// 예상하지 못한 오류
				else
				{
					char err_buff[100];
					sprintf_s(err_buff, sizeof err_buff, "Recv()에서 알 수 없는 에러.\n ERRORCODE : %d", GetLastError());
					cout << "이상에러" << endl;
					WriteLog(err_buff, "Recv Error");
				}

				if (g_Members[i]._status == PS_VALID)
				{
					g_Members[i]._status = PS_INVALID;
					g_DeleteQueue.push(DeleteJob{ i });
				}
				continue;
				// SEND시도, 연결 끊어지면 CLOSESOCKET QUEUEING
			}

			// 2. directsize만큼 한번만 전송. 나머지는 다음 프레임에 전송
			g_Members[i]._sendBuf._dataSize -= retval;
			g_Members[i]._sendBuf.MoveHead(retval);

			// 3. 이번에 전부 보냈으면, clearbuffer로 포인터 초기화.
			//if (g_Members[i]._sendBuf.IsEmpty())
			//{
			//	g_Members[i]._sendBuf.ClearBuffer();
			//}
		}
	}
	return true;
}

// 플레이어 참여에 대한 처리
void PlayerIsJoined(SOCKET& clientSock, SOCKADDR_IN& clientAddr)
{
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

	g_Members[id]._player._act = ACT_IDLE;
	g_Members[id]._player._dir = dfPACKET_MOVE_DIR_LL;
	g_Members[id]._player._x = (rand() % (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT)) + dfRANGE_MOVE_LEFT;
	g_Members[id]._player._y = (rand() % (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP)) + dfRANGE_MOVE_TOP;;
	g_Members[id]._player._hp = 100;
	g_Members[id]._is_payload = false;
	g_numMembers++;

	InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
	cout << "[CON] " << ip_buffer << ":" << ntohs(g_Members[id].addr.sin_port) <<
		"님의 접속.(연결중 : " << g_numMembers << ")" << endl << endl;

	Proxy proxy;
	// 1. client id 할당 및 캐릭터 생성
	proxy.ServerReqCreateMyCharacter(id, SENDMODE_UNI,
		id,
		g_Members[id]._player._dir,
		g_Members[id]._player._x,
		g_Members[id]._player._y,
		g_Members[id]._player._hp);
	

	// 2. 다른 참여자에게 해당플레이어 캐릭터 생성
	proxy.ServerReqCreateOtherCharacter(id, SENDMODE_BROAD,
		id,
		g_Members[id]._player._dir,
		g_Members[id]._player._x,
		g_Members[id]._player._y,
		g_Members[id]._player._hp);


	// 여기서 플레이어 상태까지 보내야함.
	// 3. 접속자에게 내부 인원들에 대한 위치, 상태 전달
	for (int i = 0;i < TOTAL_PLAYER;i++)
	{
		// 현재 존재하거나, 삭제 대기중인 유저.
		if (g_Members[i]._status != PS_EMPTY && g_Members[i]._id != id)
		{
			proxy.ServerReqCreateOtherCharacter(id, SENDMODE_UNI,
				g_Members[i]._id,
				g_Members[i]._player._dir,
				g_Members[i]._player._x,
				g_Members[i]._player._y,
				g_Members[i]._player._hp);

			// 애니메이션 동기화는 신경쓰지 않는다.

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

void DeleteSockets()
{
	// 연결하자마자 끊기면
	// connect에서 한번, read에서 rst로 또한번
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
			"님의 접속 종료. (연결중 : " << g_numMembers << ")" << endl << endl;
	
		// 죽은 캐릭터 제거
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
		// 헤더 이상
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

// 프레임 처리해야 판정됨.
void Att1Update(int id)
{
	Proxy proxy;

	// 방향이상 검사.
	if (g_Members[id]._player._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._player._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "%d님의 방향 이상. (%d).", id, g_Members[id]._player._dir);
		WriteLog(err_buff,getStackTrace(1).c_str());

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
					cout << i << "님 사망." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// 데미지 계산
				
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

	// 방향이상 검사.
	if (g_Members[id]._player._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._player._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "%d님의 방향 이상. (%d).", id, g_Members[id]._player._dir);
		WriteLog(err_buff, getStackTrace(1).c_str());

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
					cout << i << "님 사망." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// 데미지 계산

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

	// 방향이상 검사.
	if (g_Members[id]._player._dir != dfPACKET_MOVE_DIR_LL && g_Members[id]._player._dir != dfPACKET_MOVE_DIR_RR)
	{
		char err_buff[100];
		sprintf_s(err_buff, sizeof err_buff, "%d님의 방향 이상. (%d).", id, g_Members[id]._player._dir);
		WriteLog(err_buff, getStackTrace(1).c_str());

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
					cout << i << "님 사망." << endl;
					if (g_Members[i]._status == PS_VALID)
					{
						g_Members[i]._status = PS_INVALID;
						g_DeleteQueue.push(DeleteJob{ i });
					}
				}

				// 데미지 계산

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