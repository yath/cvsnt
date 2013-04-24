// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define STRICT
#include <windows.h>
#define SECURITY_WIN32
#include <security.h>
#include <ntsecapi.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lm.h>

#include "gssapi.h"
#include "krb5.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>
