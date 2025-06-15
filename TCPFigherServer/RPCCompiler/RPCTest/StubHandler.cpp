#include <list>

#pragma comment(lib, "ws2_32")
#include <WS2tcpip.h>

#include "StubHandler.h"
#include "Proxy.h"
#include "SendPacket.h"
#include "PacketDefine.h"
#include "WriteLogTXT.h"
#include "PrintStackInfo.h"

#include <list>

int StubHandler::_now_id;

int StubHandler::ServerReqCreateMyCharacter(int id, BYTE dir, short x, short y, BYTE hp)
{
    return 0;
}

int StubHandler::ServerReqCreateOtherCharacter(int id, BYTE dir, short x, short y, BYTE hp)
{
    return 0;
}

int StubHandler::ServerReqDeleteCharacter(int id)
{
    return 0;
}

int StubHandler::ClientReqMoveStart(BYTE dir, short x, short y)
{
    int id = _now_id;
    Proxy proxy;

    g_Members[id]._player._act = ACT_MOVESTART;
    g_Members[id]._player._dir = dir;
    g_Members[id]._player._x = x;
    g_Members[id]._player._y = y;

    proxy.ServerReqMoveStart(id, SENDMODE_BROAD, id, dir, x, y);

    return 0;
}

int StubHandler::ServerReqMoveStart(int id, BYTE dir, short x, short y)
{
    return 0;
}

int StubHandler::ClientReqMoveStop(BYTE dir, short x, short y)
{
    Proxy proxy;
    int id = StubHandler::_now_id;
    char ip_buffer[20];

    if (dir != dfPACKET_MOVE_DIR_LL && dir != dfPACKET_MOVE_DIR_RR)
    {
        InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
        char err_buff[100];
        sprintf_s(err_buff, sizeof err_buff, "id : %d님의 방향 이상. (%d).(연결 종료)", id, dir);
        WriteLog(err_buff, ip_buffer, getStackTrace(1).c_str());

        if (g_Members[id]._status == PS_VALID)
        {
            g_Members[id]._status = PS_INVALID;
            g_DeleteQueue.push(DeleteJob{ id });
        }

        return 0;
    }

    g_Members[id]._player._act = ACT_IDLE;
    g_Members[id]._player._dir = dir;
    g_Members[id]._player._x = x;
    g_Members[id]._player._y = y;

    proxy.ServerReqMoveStop(id, SENDMODE_BROAD, id, dir, x, y);

    return 0;
}

int StubHandler::ServerReqMoveStop(int id, BYTE dir, short x, short y)
{
    return 0;
}

int StubHandler::ClientReqAttack1(BYTE dir, short x, short y)
{
    Proxy proxy;
    int id = StubHandler::_now_id;
    char ip_buffer[20];

    if (dir != dfPACKET_MOVE_DIR_LL && dir != dfPACKET_MOVE_DIR_RR)
    {
        InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
        char err_buff[100];
        sprintf_s(err_buff, sizeof err_buff, "id : %d님의 방향 이상. (%d).(연결 종료)", id, dir);
        WriteLog(err_buff, ip_buffer, getStackTrace(1).c_str());

        if (g_Members[id]._status == PS_VALID)
        {
            g_Members[id]._status = PS_INVALID;
            g_DeleteQueue.push(DeleteJob{ id });
        }

        return 0;
    }
    // 1. 공격요청
    g_Members[id]._player._act = ACT_ATT1;
    g_Members[id]._player._dir = dir;
    g_Members[id]._player._x = x;
    g_Members[id]._player._y = y;
    // 2. broadcast
    proxy.ServerReqAttack1(id, SENDMODE_BROAD, id, dir, x, y);

    return 0;
}

int StubHandler::ServerReqAttack1(int id, BYTE dir, short x, short y)
{
    return 0;
}

int StubHandler::ClientReqAttack2(BYTE dir, short x, short y)
{
    Proxy proxy;
    int id = StubHandler::_now_id;
    char ip_buffer[20];

    if (dir != dfPACKET_MOVE_DIR_LL && dir != dfPACKET_MOVE_DIR_RR)
    {
        InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
        char err_buff[100];
        sprintf_s(err_buff, sizeof err_buff, "id : %d님의 방향 이상. (%d).(연결 종료)", id, dir);
        WriteLog(err_buff, ip_buffer, getStackTrace(1).c_str());

        if (g_Members[id]._status == PS_VALID)
        {
            g_Members[id]._status = PS_INVALID;
            g_DeleteQueue.push(DeleteJob{ id });
        }

        return 0;
    }
    // 1. 공격요청
    g_Members[id]._player._act = ACT_ATT2;
    g_Members[id]._player._dir = dir;
    g_Members[id]._player._x = x;
    g_Members[id]._player._y = y;
    // 2. broadcast
    proxy.ServerReqAttack2(id, SENDMODE_BROAD, id, dir, x, y);

    return 0;
}

int StubHandler::ServerReqAttack2(int id, BYTE dir, short x, short y)
{
    return 0;
}

int StubHandler::ClientReqAttack3(BYTE dir, short x, short y)
{
    Proxy proxy;
    int id = StubHandler::_now_id;
    char ip_buffer[20];

    if (dir != dfPACKET_MOVE_DIR_LL && dir != dfPACKET_MOVE_DIR_RR)
    {
        InetNtopA(AF_INET, &g_Members[id].addr.sin_addr, ip_buffer, sizeof ip_buffer);
        char err_buff[100];
        sprintf_s(err_buff, sizeof err_buff, "id : %d님의 방향 이상. (%d).(연결 종료)", id, dir);
        WriteLog(err_buff, ip_buffer, getStackTrace(1).c_str());

        if (g_Members[id]._status == PS_VALID)
        {
            g_Members[id]._status = PS_INVALID;
            g_DeleteQueue.push(DeleteJob{ id });
        }

        return 0;
    }
    // 1. 공격요청
    g_Members[id]._player._act = ACT_ATT3;
    g_Members[id]._player._dir = dir;
    g_Members[id]._player._x = x;
    g_Members[id]._player._y = y;
    // 2. broadcast
    proxy.ServerReqAttack3(id, SENDMODE_BROAD, id, dir, x, y);

    return 0;
}

int StubHandler::ServerReqAttack3(int id, BYTE dir, short x, short y)
{
    return 0;
}

int StubHandler::ServerReqDamage(int idAttacker, int idDefender, BYTE hpDefender)
{
    return 0;
}

