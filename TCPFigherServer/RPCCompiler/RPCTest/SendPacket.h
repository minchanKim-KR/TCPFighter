#pragma once
#include <Windows.h>

// SendMode Mask for RPC fucntions
// 0001
#define SENDMODE_UNI (BYTE)1
// 0010
#define SENDMODE_BROAD (BYTE)2

// SendUnicast, SendBroadcast return value
// Á¤»ó : 0
#define RINGBUF_FULLED 402
#define RINGBUF_ABNORMAL 403

int SendUnicast(int id, char* packet, int len);
int SendBroadcast(int id, char* packet, int len);


