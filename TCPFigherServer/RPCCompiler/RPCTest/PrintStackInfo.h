#pragma once
#include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "dbghelp.lib")

// 런타임에 Stack정보를 가져와 출력.

std::string getStackTrace(int skip);