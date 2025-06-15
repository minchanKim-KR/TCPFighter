#pragma once
// 네트워크 패킷
#include <Windows.h>

#pragma pack(push, 1)
struct Header
{
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입. 
};
#pragma pack(pop)