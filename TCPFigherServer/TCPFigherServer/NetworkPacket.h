#pragma once
// 네트워크 패킷

#pragma pack(push, 1)
struct Header
{
	BYTE	byCode;			// 패킷코드 0x89 고정.
	BYTE	bySize;			// 패킷 사이즈.
	BYTE	byType;			// 패킷타입. 
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestCreatePlayer
{
	int _id;
	BYTE _dir;
	short _x;
	short _y;
	BYTE _hp;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DeletePlayer
{
	int _id;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestMoveStart
{
	int _id;
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ClientRequestMoveStart
{
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestMoveStop
{
	int _id;
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ClientRequestMoveStop
{
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestAtt1
{
	int _id;
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ClientRequestAtt1
{
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestAtt2
{
	int _id;
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ClientRequestAtt2
{
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestAtt3
{
	int _id;
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ClientRequestAtt3
{
	BYTE _dir;
	short _x;
	short _y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ServerRequestDamageStep
{
	int _id_attacker;
	int _id_defender;
	BYTE _hp_defender;
};
#pragma pack(pop)