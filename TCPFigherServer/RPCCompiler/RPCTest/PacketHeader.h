#pragma once
// ��Ʈ��ũ ��Ŷ
#include <Windows.h>

#pragma pack(push, 1)
struct Header
{
	BYTE	byCode;			// ��Ŷ�ڵ� 0x89 ����.
	BYTE	bySize;			// ��Ŷ ������.
	BYTE	byType;			// ��ŶŸ��. 
};
#pragma pack(pop)