// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#if OS == OS_WINDOWS
#   define _WINSOCK_DEPRECATED_NO_WARNINGS

#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#   endif

#   include <winsock2.h>
#   include <Ws2tcpip.h>
#   pragma comment(lib, "Ws2_32.lib")

#   include <MSWSock.h>
#   pragma comment(lib, "Mswsock.lib")

#   include <iphlpapi.h>
#   pragma comment(lib, "Iphlpapi.lib")

// Windows Header Files
#   include <windows.h>

#   include <basetsd.h>

#   define MALLOC(x)       HeapAlloc(GetProcessHeap(), 0, (x))
#   define FREE(x)         HeapFree(GetProcessHeap(), 0, (x))

#   define NEWLINE "\r\n"

#   define DllExport __declspec(dllexport)
#   define DllImport __declspec(dllimport)

#elif OS == OS_LINUX

#   include <stdlib.h>
#   include <stdio.h>
#   include <errno.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <sys/time.h>
#   include <arpa/inet.h>
#   include <poll.h>
#   include <unistd.h>

#   define NEWLINE "\n"

#   define DllExport
#   define DllImport

typedef int SOCKET;
typedef struct sockaddr SOCKADDR;
#endif

#ifndef INVALID_SOCKET
#   define INVALID_SOCKET (-1)
#endif


// reference additional headers your program requires here
#include <string>
#include <iostream>
#include <sstream>
#include <mutex>


#ifndef _Override
#   define _Override
#endif