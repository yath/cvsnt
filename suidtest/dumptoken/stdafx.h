// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#define _WIN32_WINNT 0x0500
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <lm.h>
#include <lmcons.h>
#include <winsock2.h>
#include <dbghelp.h>
#include <objbase.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <process.h>
#include <ntsecapi.h>
#include <tchar.h>
#include <sddl.h>

#include <stdio.h>
