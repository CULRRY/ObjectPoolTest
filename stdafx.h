#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#include <Windows.h>
#include <iostream>
using namespace std;

#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <ws2tcpip.h>

#include "Types.h"
#include "Macro.h"
