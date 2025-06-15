#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Winmm.lib")
#include <WS2tcpip.h>

#include <list>
#include <fstream>
#include <Windows.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Stub.h"
#include "Proxy.h"

#include "SendPacket.h"
#include "StubHandler.h"

#include "PrintStackInfo.h"

using namespace std;

#define SERIALIZING_BUF_FULLED 401
#define RINGBUF_FULLED 402

void MakeProxyHeaderFile()
{
    ofstream proxy_h;
    proxy_h.open("Proxy.h");

    if (!proxy_h.is_open())
    {
        cout << "proxy.h를 열 수없습니다." << endl;
        return;
    }

    using json = nlohmann::json;

    std::ifstream f("RPC.json");
    json data = json::parse(f);

    json functions_json = data["Functions"];

    // Function Name
    std::vector<string> funcs_name;
    std::vector<json> funcs_json;

    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_name.push_back(it.value().begin().key());
    }
    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_json.push_back(it.value());
    }

    ////////////////////////////////////////////
    // .h
    ////////////////////////////////////////////
    proxy_h << "#pragma once\n";
    proxy_h << "#include <Windows.h>\n";
    proxy_h << "#include <list>\n";
    proxy_h << "#include \"Session.h\"\n";
    proxy_h << "#include \"RingBuffer.h\"\n";


    proxy_h << "\n";
    proxy_h << "using namespace std;\n";
    proxy_h << "\n";

    proxy_h << "class Proxy\n";
    proxy_h << "{" << "\n";
    proxy_h << "public:" << "\n";

    // 함수
    for (int i_func = 0;i_func < funcs_name.size();i_func++)
    {
        // type
        string packet_type = funcs_json[i_func][funcs_name[i_func].c_str()]["type"].get<string>();
        // return
        string ret_type = funcs_json[i_func][funcs_name[i_func].c_str()]["return"].get<string>();
        // params
        std::vector<std::pair<string, string>> params;

        json param = funcs_json[i_func][funcs_name[i_func].c_str()]["params"];
        std::vector<json> params_json;

        for (auto it = param.begin(); it != param.end(); ++it) {
            params_json.push_back(it.value());
        }

        for (int i = 0;i < params_json.size();i++)
        {
            string param_key = params_json[i].begin().key();
            string param_val = params_json[i].begin().value().get<string>();

            params.emplace_back(param_key, param_val);
        }

        // 함수 작성
        // return value
        proxy_h << "\t" << ret_type << " ";

        // function name
        proxy_h << funcs_name[i_func] << "(";
        proxy_h << "int sessionId, BYTE sendMode, ";

        for (int i_param = 0;i_param < params.size();i_param++)
        {
            if (!strncmp(params[i_param].second.c_str(), "list", 4))
                proxy_h << params[i_param].second << "&" << " " << params[i_param].first;
            else
                proxy_h << params[i_param].second << " " << params[i_param].first;

            if (i_param + 1 == params.size())
                break;
            else
                proxy_h << ",";
        }
        proxy_h << ");\n";
    }
    proxy_h << "};" << "\n";
    proxy_h << "\n";
    proxy_h << "\n";
}
void MakeProxyCppFile()
{
    ofstream proxy_cpp;
    proxy_cpp.open("Proxy.cpp");

    if (!proxy_cpp.is_open())
    {
        cout << "proxy.cpp를 열 수없습니다." << endl;
        return;
    }

    using json = nlohmann::json;

    std::ifstream f("RPC.json");
    json data = json::parse(f);

    json functions_json = data["Functions"];

    // Function Name
    std::vector<string> funcs_name;
    std::vector<json> funcs_json;

    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_name.push_back(it.value().begin().key());
    }
    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_json.push_back(it.value());
    }

    ////////////////////////////////////////////
    // .cpp
    ////////////////////////////////////////////
    proxy_cpp << "#pragma once\n";
    proxy_cpp << "#pragma comment(lib, \"ws2_32\")\n";
    proxy_cpp << "#pragma comment(lib,\"Winmm.lib\")\n";
    proxy_cpp << "#include <WinSock2.h>\n";
    proxy_cpp << "#include <WS2tcpip.h>\n";
    proxy_cpp << "\n";
    proxy_cpp << "#include \"Proxy.h\"\n";
    proxy_cpp << "#include \"PacketDefine.h\"\n";
    proxy_cpp << "#include \"PacketHeader.h\"\n";
    proxy_cpp << "#include \"SerializingBuffer.h\"\n";
    proxy_cpp << "#include \"RingBuffer.h\"\n";
    proxy_cpp << "#include \"SendPacket.h\"\n";
    proxy_cpp << "#include \"Session.h\"\n";
    proxy_cpp << "#include \"WriteLogTXT.h\"\n";
    proxy_cpp << "#include \"PrintStackInfo.h\"\n";
    proxy_cpp << "\n";
    proxy_cpp << "\n";

    proxy_cpp << "#define SERIALIZING_BUF_FULLED " << SERIALIZING_BUF_FULLED << "\n";

    proxy_cpp << "\n";
    proxy_cpp << "\n";

    // 함수
    for (int i_func = 0;i_func < funcs_name.size();i_func++)
    {
        // type
        string packet_type = funcs_json[i_func][funcs_name[i_func].c_str()]["type"].get<string>();
        // return
        string ret_type = funcs_json[i_func][funcs_name[i_func].c_str()]["return"].get<string>();
        // params
        std::vector<std::pair<string, string>> params;

        json param = funcs_json[i_func][funcs_name[i_func].c_str()]["params"];
        std::vector<json> params_json;

        for (auto it = param.begin(); it != param.end(); ++it) {
            params_json.push_back(it.value());
        }

        for (int i = 0;i < params_json.size();i++)
        {
            string param_key = params_json[i].begin().key();
            string param_val = params_json[i].begin().value().get<string>();

            params.emplace_back(param_key, param_val);
        }

        // 함수 작성
        // return value
        proxy_cpp << ret_type << " ";

        // function name
        proxy_cpp << "Proxy::";
        proxy_cpp << funcs_name[i_func] << "(";
        proxy_cpp << "int sessionId, BYTE sendMode,";

        for (int i_param = 0;i_param < params.size();i_param++)
        {
            if (!strncmp(params[i_param].second.c_str(), "list", 4))
                proxy_cpp << params[i_param].second << "&" << " " << params[i_param].first;
            else
                proxy_cpp << params[i_param].second << " " << params[i_param].first;

            if (i_param + 1 == params.size())
                break;
            else
                proxy_cpp << ",";
        }
        proxy_cpp << ")\n";

        // Function Body
        proxy_cpp << "{\n";
        // 1. 헤더의 크기 (미리 정의됨)만큼 배열을 만들어서 값을 채움. (Packet Define에 의존)
        // 2. 헤더의 headercode, payload size , packet type

        proxy_cpp << "\t" << "SerializingBuffer sb;\n";
        proxy_cpp << "\t" << "char ip_buffer[20];\n";
        proxy_cpp << "\t" << "char err_buff1[100];\n";
        proxy_cpp << "\t" << "char err_buff2[100];\n";
        for (int i_param = 0;i_param < params.size();i_param++)
        {
            // List인 경우
            if (!strncmp(params[i_param].second.c_str(), "list", 4))
            {
                string param_key = params[i_param].first;
                proxy_cpp << "\tsb << " << params[i_param + 1].first << ";\n";
                proxy_cpp << "\tsb.PutData(" << params[i_param].first << ", ";
                proxy_cpp << params[i_param + 1].first.c_str() << ");\n";
                i_param++;
            }
            else if ((!strcmp(params[i_param].second.c_str(), "char*") || 
                !strcmp(params[i_param].second.c_str(), "const char*")) && 
                !strcmp(params[i_param + 1].second.c_str(), "int"))
            {
                proxy_cpp << "\tsb << " << params[i_param + 1].first << ";\n";
                proxy_cpp << "\tsb.PutData(" << params[i_param].first.c_str() << ", ";
                proxy_cpp << params[i_param + 1].first.c_str() << ");\n";
                i_param++;
            }
            else
            {
                proxy_cpp << "\tsb << " << params[i_param].first << ";\n";
            }

        }

        proxy_cpp << "\t" << "if(sb.Failed())\n";
        proxy_cpp << "\t{\n";
        proxy_cpp << "\t\tif (g_Members[sessionId]._status == PS_VALID)\n";
        proxy_cpp << "\t\t{\n";
        proxy_cpp << "\t\t\tInetNtopA(AF_INET, &g_Members[sessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);\n";
        proxy_cpp << "\t\t\tsprintf_s(err_buff1, sizeof err_buff1, \"%s, SB에 데이터 삽입 실패(연결해제)\", ip_buffer);\n";
        proxy_cpp << "\t\t\tsprintf_s(err_buff2, sizeof err_buff2, \"BufferSize : %d, Capacity : %d, DataSize : %d\",sb.GetBufferSize(),sb.GetCapacity(),sb.GetDataSize());\n";
        proxy_cpp << "\t\t\tWriteLog(err_buff1,err_buff2, getStackTrace(1).c_str());\n";
        proxy_cpp << "\t\t\tg_Members[sessionId]._status = PS_INVALID;\n";
        proxy_cpp << "\t\t\tg_DeleteQueue.push(DeleteJob{ sessionId });\n";
        proxy_cpp << "\t\t}\n";
        proxy_cpp << "\t\treturn SERIALIZING_BUF_FULLED;\n";
        proxy_cpp << "\t}\n";

        proxy_cpp << "\t" << "Header header;\n";
        proxy_cpp << "\t" << "header.byCode = " << "HEADER_CODE;\n";
        proxy_cpp << "\t" << "header.bySize = " << "sb.GetDataSize();\n";
        proxy_cpp << "\t" << "header.byType = " << packet_type << ";\n";
        // 3. 직렬화, SerializingBuffer.h에 의존
        // 4. 링버퍼에 헤더와 직렬화 버퍼 내용물 삽입.

        proxy_cpp << "\t" << "if(sendMode & SENDMODE_UNI)";
        proxy_cpp << "\t{\n";
        proxy_cpp << "\t\tSendUnicast(sessionId, (char*)&header, sizeof(header));\n";
        proxy_cpp << "\t\tSendUnicast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());\n";
        proxy_cpp << "\t}\n";
        proxy_cpp << "\t" << "if(sendMode & SENDMODE_BROAD)";
        proxy_cpp << "\t{\n";
        proxy_cpp << "\t\tSendBroadcast(sessionId, (char*)&header, sizeof(header));\n";
        proxy_cpp << "\t\tSendBroadcast(sessionId, (char*)sb.GetBufferPtr(), sb.GetDataSize());\n";
        proxy_cpp << "\t}\n";
        proxy_cpp << "\t" << "else";
        proxy_cpp << "\t{\n";
        proxy_cpp << "\t\treturn -1;\n";
        proxy_cpp << "\t}\n";

        proxy_cpp << "\treturn 0;";

        proxy_cpp << "\n}\n";
    }
}
void MakeStubHeaderFile()
{
    ofstream stub_h;
    stub_h.open("Stub.h");

    if (!stub_h.is_open())
    {
        cout << "Stub.h를 열 수없습니다." << endl;
        return;
    }

    using json = nlohmann::json;

    std::ifstream f("RPC.json");
    json data = json::parse(f);

    json functions_json = data["Functions"];

    // Function Name
    std::vector<string> funcs_name;
    std::vector<json> funcs_json;

    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_name.push_back(it.value().begin().key());
    }
    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_json.push_back(it.value());
    }

    ////////////////////////////////////////////
    // .h
    ////////////////////////////////////////////
    stub_h << "#pragma once\n";
    stub_h << "#include <Windows.h>\n";
    stub_h << "#include <list>\n";
    stub_h << "#include \"Session.h\"\n";
    stub_h << "#include \"RingBuffer.h\"\n";
    stub_h << "#include \"PacketHeader.h\"\n";


    stub_h << "\n";
    stub_h << "using namespace std;\n";
    stub_h << "\n";

    stub_h << "\n";

    stub_h << "class Stub\n";
    stub_h << "{" << "\n";
    stub_h << "public:" << "\n";
    stub_h << "\tstatic void RPC_PacketProcedure(int SessionId,Stub* handle);\n";
    stub_h << "\n";

    // 함수
    for (int i_func = 0;i_func < funcs_name.size();i_func++)
    {
        // type
        string packet_type = funcs_json[i_func][funcs_name[i_func].c_str()]["type"].get<string>();
        // return
        string ret_type = funcs_json[i_func][funcs_name[i_func].c_str()]["return"].get<string>();
        // params
        std::vector<std::pair<string, string>> params;

        json param = funcs_json[i_func][funcs_name[i_func].c_str()]["params"];
        std::vector<json> params_json;

        for (auto it = param.begin(); it != param.end(); ++it) {
            params_json.push_back(it.value());
        }

        for (int i = 0;i < params_json.size();i++)
        {
            string param_key = params_json[i].begin().key();
            string param_val = params_json[i].begin().value().get<string>();

            params.emplace_back(param_key, param_val);
        }

        // 함수 작성
        // return value
        stub_h << "\t" << "virtual" << " " << ret_type << " ";

        // function name
        stub_h << funcs_name[i_func] << "(";

        // parameters
        for (int i_param = 0;i_param < params.size();i_param++)
        {
            if(!strncmp(params[i_param].second.c_str(), "list", 4))
                stub_h << params[i_param].second << "&" << " " << params[i_param].first;
            else
                stub_h << params[i_param].second << " " << params[i_param].first;

            if (i_param + 1 == params.size())
                break;
            else
                stub_h << ",";
        }
        stub_h << ") = 0;\n";
    }
    stub_h << "};" << "\n";
    stub_h << "\n";
    stub_h << "\n";
}
void MakeStubCppFile()
{
    ofstream stub_cpp;
    stub_cpp.open("Stub.cpp");

    if (!stub_cpp.is_open())
    {
        cout << "stub.cpp를 열 수없습니다." << endl;
        return;
    }

    using json = nlohmann::json;

    std::ifstream f("RPC.json");
    json data = json::parse(f);

    json functions_json = data["Functions"];

    // Function Name
    std::vector<string> funcs_name;
    std::vector<json> funcs_json;

    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_name.push_back(it.value().begin().key());
    }
    for (auto it = functions_json.begin(); it != functions_json.end(); ++it) {
        funcs_json.push_back(it.value());
    }

    ////////////////////////////////////////////
    // .cpp
    ////////////////////////////////////////////
    stub_cpp << "#pragma once\n";
    stub_cpp << "#pragma comment(lib, \"ws2_32\")\n";
    stub_cpp << "#pragma comment(lib,\"Winmm.lib\")\n";
    stub_cpp << "#include <WinSock2.h>\n";
    stub_cpp << "#include <WS2tcpip.h>\n";
    stub_cpp << "\n";
    stub_cpp << "#include \"PacketDefine.h\"\n";
    stub_cpp << "#include \"PacketHeader.h\"\n";
    stub_cpp << "#include \"RingBuffer.h\"\n";
    stub_cpp << "#include \"SerializingBuffer.h\"\n";
    stub_cpp << "#include \"RingBuffer.h\"\n";
    stub_cpp << "#include \"Session.h\"\n";
    stub_cpp << "#include \"WriteLogTXT.h\"\n";
    stub_cpp << "#include \"PrintStackInfo.h\"\n";
    stub_cpp << "#include \"Stub.h\"\n";
    stub_cpp << "#include \"StubHandler.h\"\n";

    stub_cpp << "\n";
    stub_cpp << "\n";

    stub_cpp << "char g_recvStr[MAX_PAYLOAD_SIZE];\n";

    stub_cpp << "\n";
    stub_cpp << "\n";

    stub_cpp << "void Stub::RPC_PacketProcedure(int SessionId, Stub* handle)\n";
    stub_cpp << "{\n";
    stub_cpp << "\tchar ip_buffer[20];\n";
    stub_cpp << "\tchar err_buff1[100];\n";
    stub_cpp << "\tchar err_buff2[100];\n";
    stub_cpp << "\tRingBuffer& recvbuf = g_Members[SessionId]._recvBuf;\n";
    stub_cpp << "\tHeader h;\n";
    stub_cpp << "\tSerializingBuffer sb;\n";

    // 헤더
    stub_cpp << "\trecvbuf.Dequeue((char*)&h, sizeof(Header));\n";
    stub_cpp << "\n";
    stub_cpp << "\tbool resizing;\n";
    stub_cpp << "\n";
    stub_cpp << "\tif (sb.GetBufferSize() < h.bySize)\n";
    stub_cpp << "\t\tresizing = sb.Resize(h.bySize);\n";

    stub_cpp << "\tif (resizing == false)\n";
    stub_cpp << "\t{\n";
    stub_cpp << "\t\tsprintf_s(err_buff1, \"sb.BufferSize : % d\", sb.GetBufferSize());\n";
    stub_cpp << "\t\tWriteLog(\"SerializingBuffer.Resize 실패.\", err_buff1,getStackTrace(1).c_str());\n";
    stub_cpp << "\n";
    stub_cpp << "\t\tif (g_Members[SessionId]._status == PS_VALID)\n";
    stub_cpp << "\t\t{\n";
    stub_cpp << "\t\t\tInetNtopA(AF_INET, &g_Members[SessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);\n";
    stub_cpp << "\t\t\tsprintf_s(err_buff2, sizeof err_buff2, \"%s, SerializingBuffer Resizing 실패(연결해제)\", ip_buffer);\n";
    stub_cpp << "\t\t\tWriteLog(err_buff2);\n";
    stub_cpp << "\t\t\tg_Members[SessionId]._status = PS_INVALID;\n";
    stub_cpp << "\t\t\tg_DeleteQueue.push(DeleteJob{ SessionId });\n";
    stub_cpp << "\t\t}\n";
    stub_cpp << "\t\treturn;\n";
    stub_cpp << "\t}\n";

    // Serializing Buffer에 payload를 담는다.
    stub_cpp << "\n";
    stub_cpp << "\trecvbuf.Dequeue((char*)sb.GetBufferPtr(), h.bySize);";
    stub_cpp << "\tsb.MoveWritePos(h.bySize);";
    stub_cpp << "\n";

    // 분기로 처리
    stub_cpp << "\tswitch (h.byType)\n";
    stub_cpp << "\t\t{\n";
    // 함수
    for (int i_func = 0;i_func < funcs_name.size();i_func++)
    {
        // type
        string packet_type = funcs_json[i_func][funcs_name[i_func].c_str()]["type"].get<string>();
        // return
        string ret_type = funcs_json[i_func][funcs_name[i_func].c_str()]["return"].get<string>();
        // params
        std::vector<std::pair<string, string>> params;

        json param = funcs_json[i_func][funcs_name[i_func].c_str()]["params"];
        std::vector<json> params_json;

        for (auto it = param.begin(); it != param.end(); ++it) {
            params_json.push_back(it.value());
        }

        for (int i = 0;i < params_json.size();i++)
        {
            string param_key = params_json[i].begin().key();
            string param_val = params_json[i].begin().value().get<string>();

            params.emplace_back(param_key, param_val);
        }

        // 함수 작성
        // return value
        stub_cpp << "\tcase " << packet_type.c_str() << ":\n";
        stub_cpp << "\t\t{\n";
        for (int i = 0;i < params.size();i++)
        {
            if (!strcmp(params[i].second.c_str(), "char*") ||
                !strcmp(params[i].second.c_str(), "const char*"))
            {
                continue;
            }

            stub_cpp << "\t\t" 
                << params[i].second.c_str() << " " 
                << params[i].first.c_str() << ";\n";
        }

        //////////////////////////////////////////////////
        // RPC STUB으로 넘겨줄 함수 매개변수 생성, 
        // 단 char*, const char*는 미리 정의된 recv용 배열을 넘김.
        //////////////////////////////////////////////////
        for (int i = 0;i < params.size();i++)
        {
            if (!strncmp(params[i].second.c_str(), "list", 4))
            {
                stub_cpp << "\t\tsb >> " << params[i + 1].first.c_str() << ";\n";
                stub_cpp << "\t\tsb.GetData(" << params[i].first.c_str() << ", " << params[i + 1].first.c_str() << ");\n";
                i++;
            }
            else if (!strcmp(params[i].second.c_str(), "char*") || 
                !strcmp(params[i].second.c_str(), "const char*"))
            {
                stub_cpp << "\t\tsb >> " << params[i + 1].first.c_str() << ";\n";
                stub_cpp << "\t\tsb.GetData(g_recvStr, " << params[i + 1].first.c_str() << ");\n";
                i++;
            }
            else
            {
                stub_cpp << "\t\tsb >> "
                    << params[i].first.c_str() << ";\n";
            }
        }

        // stub 함수 호출부
        stub_cpp << "\t\thandle->" << funcs_name[i_func].c_str() << "(";
        for (int i = 0;i < params.size();i++)
        {
            if (!strcmp(params[i].second.c_str(), "char*") ||
                !strcmp(params[i].second.c_str(), "const char*"))
            {
                stub_cpp << "g_recvStr";
            }
            else
            {
                stub_cpp << params[i].first.c_str();
            }

            if (i + 1 == params.size())
                break;
            else
                stub_cpp << ",";
        }
        stub_cpp << ");\n";
        stub_cpp << "\t\tbreak;\n";
        stub_cpp << "\t}\n";
    }
    stub_cpp << "\tdefault:\n";
    stub_cpp << "\t\t{\n";
    stub_cpp << "\t\tInetNtopA(AF_INET, &g_Members[SessionId].addr.sin_addr, ip_buffer, sizeof ip_buffer);\n";
    stub_cpp << "\t\tsprintf_s(err_buff1, sizeof err_buff1, \"%s, 정의되지 않은 패킷타입.\", ip_buffer);\n";
    stub_cpp << "\t\tsprintf_s(err_buff2, sizeof err_buff2, \"Packet Type : %d\", h.byType);\n";
    stub_cpp << "\t\tWriteLog(err_buff1, err_buff2, getStackTrace(1).c_str());\n";
    stub_cpp << "\n";
    
    stub_cpp << "\t\tif (g_Members[SessionId]._status == PS_VALID)\n";
    stub_cpp << "\t\t{\n";
    stub_cpp << "\t\t\tg_Members[SessionId]._status = PS_INVALID;\n";
    stub_cpp << "\t\t\tg_DeleteQueue.push(DeleteJob{ SessionId });\n";
    stub_cpp << "\t\t}\n";
    stub_cpp << "\t\tbreak;\n";
    stub_cpp << "\t\t}\n";
    stub_cpp << "\t}\n";
    stub_cpp << "}\n";
}

// 함수 맵을 생성하여 문자열 이름으로 함수 호출
void main() 
{    
    char asc[100];
    GetCurrentDirectoryA(100, asc);
    cout << asc << endl;

    MakeProxyHeaderFile();
    MakeProxyCppFile();
    MakeStubHeaderFile();
    MakeStubCppFile();
   
    // 복원

    cout << "작업 완료. 아무 문자나 입력해주세요." << endl;
    char temp;
    cin >> temp;
}
