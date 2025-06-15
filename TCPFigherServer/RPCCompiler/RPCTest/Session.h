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
	// ���� ���� ( ����x, ������, ���� ���)
	PS_Status _status;

	// Content�� ����ϴ� ���� ����
	Player _player;

	// ��Ŷ �۾� ����.
	BOOL _is_payload; // payload �۾���
	int _h_type;

	// �ۼ��� ������
	RingBuffer _recvBuf;
	RingBuffer _sendBuf;

	SOCKET sock;
	SOCKADDR_IN addr;
	ULONGLONG _tStamp;

	int _id;
};

extern PlayerSession g_Members[TOTAL_PLAYER];
extern std::queue<DeleteJob> g_DeleteQueue;