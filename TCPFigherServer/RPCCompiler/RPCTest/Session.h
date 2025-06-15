#pragma once

#include <Windows.h>
#include <queue>

#include "RingBuffer.h"

#define TOTAL_PLAYER 60

enum PS_Status
{
	// player status
	PS_EMPTY = 0, PS_VALID, PS_INVALID,
};
enum Player_Action
{
	ACT_IDLE, ACT_MOVESTART, ACT_MOVING, ACT_ATT1, ACT_ATT2, ACT_ATT3
};

struct DeleteJob
{
	int _id;
};

struct Player
{
	Player_Action _act;
	BYTE _dir;
	BYTE _hp;
	short _y;
	short _x;
};

struct PlayerSession
{
	// 세션 상태 ( 연결x, 연결중, 삭제 대기)
	PS_Status _status;

	// Content에 사용하는 유저 상태
	Player _player;

	// 패킷 작업 지점.
	BOOL _is_payload; // payload 작업중
	int _h_type;

	// 송수신 링버퍼
	RingBuffer _recvBuf;
	RingBuffer _sendBuf;

	SOCKET sock;
	SOCKADDR_IN addr;
	ULONGLONG _tStamp;

	int _id;
};

extern PlayerSession g_Members[TOTAL_PLAYER];
extern std::queue<DeleteJob> g_DeleteQueue;