#pragma once

#define OS_UNKNOWN      0
#define OS_WINDOWS      1
#define OS_LINUX        2

#if defined WIN32
#   define OS OS_WINDOWS

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#   include <SDKDDKVer.h>
#elif defined __linux
#   define OS OS_LINUX
#else
#   define OS OS_UNKNOWN
#endif