#pragma once

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define NOMINMAX
#include <WinSock2.h>
#include <windows.h>

#endif //_WIN32

#include <stdio.h>
