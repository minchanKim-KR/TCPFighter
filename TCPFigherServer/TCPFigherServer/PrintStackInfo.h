#pragma once
#include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "dbghelp.lib")

// ��Ÿ�ӿ� Stack������ ������ ���.

std::string getStackTrace(int skip);