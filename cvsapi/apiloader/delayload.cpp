/*
	CVSNT Generic API
    Copyright (C) 2004-5 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* Win32 specific */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <DelayImp.h>
#include "apiloader.h"

static FARPROC CALLBACK delayHook(unsigned dliNotify, PDelayLoadInfo pdli);
static TCHAR cvsntLibPath[MAX_PATH];
static TCHAR cvsntInstallPath[MAX_PATH];

static int GetGlobalValue(LPCTSTR value, LPTSTR buf, DWORD len)
{
	static const LPCTSTR regkey = _T("Software\\CVS\\PServer");
	HKEY hKey;
	DWORD dwType,dwLen;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,regkey,0,KEY_READ,&hKey))
		return -1; // Couldn't open or create key

	dwType=REG_SZ;
	dwLen=len;
	if(RegQueryValueEx(hKey,value,NULL,&dwType,(LPBYTE)buf,&dwLen))
	{
		RegCloseKey(hKey);
		return -1;
	}
	RegCloseKey(hKey);
	return 0;
}	

static int GetUserValue(LPCTSTR value, LPTSTR buf, DWORD len)
{
	static const LPCTSTR regkey = _T("Software\\Cvsnt\\PServer");
	HKEY hKey;
	DWORD dwType,dwLen;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,regkey,0,KEY_READ,&hKey))
		return -1; // Couldn't open or create key

	dwType=REG_SZ;
	dwLen=len;
	if(RegQueryValueEx(hKey,value,NULL,&dwType,(LPBYTE)buf,&dwLen))
	{
		RegCloseKey(hKey);
		return -1;
	}
	RegCloseKey(hKey);
	return 0;
}	

void __apiloadHookDelayFunctions(void **notify, void **failure)
{
	*(PfnDliHook*)notify = delayHook;
	*(PfnDliHook*)failure = delayHook;

	if(GetGlobalValue(_T("LibraryPath"), cvsntLibPath, sizeof(cvsntLibPath)/sizeof(cvsntLibPath[0])) &&
		GetUserValue(_T("LibraryPath"), cvsntLibPath, sizeof(cvsntLibPath)/sizeof(cvsntLibPath[0])))
	{
		cvsntLibPath[0]='\0';
	}

	if(GetGlobalValue(_T("InstallPath"), cvsntInstallPath, sizeof(cvsntInstallPath)/sizeof(cvsntInstallPath[0])) &&
		GetGlobalValue(_T("InstallPath"), cvsntInstallPath, sizeof(cvsntInstallPath)/sizeof(cvsntInstallPath[0])))
	{
		cvsntInstallPath[0]='\0';
	}
}

static FARPROC CALLBACK delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	switch (dliNotify)
	{
		case dliStartProcessing:
			break;
		case dliNotePreLoadLibrary:
			{
				HMODULE hLib = NULL;
				TCHAR fn[MAX_PATH],path[MAX_PATH];
				MultiByteToWideChar(CP_ACP,0,pdli->szDll,-1,fn,sizeof(fn)/sizeof(fn[0]));
				GetCurrentDirectory(sizeof(path),path);

				// LOAD_WITH_ALTERED_SEARCH_PATH seems to be broken, or at
				// least not working as specified in the documentation... need
				// to verify what this means for the control panel

				if(cvsntLibPath[0])
				{
					SetCurrentDirectory(cvsntLibPath);
					hLib = LoadLibrary(fn);
					SetCurrentDirectory(path);
				}

				if(!hLib && cvsntInstallPath[0])
				{
					SetCurrentDirectory(cvsntInstallPath);
					hLib = LoadLibrary(fn);
					SetCurrentDirectory(path);
				}

				/* If hLib is NULL here standard searches are used */
				return (FARPROC)hLib;
			}
		case dliNotePreGetProcAddress:
			break;
		case dliFailLoadLib: 
//			CServerIo::trace(1,"Delayload of %s failed!",pdli->szDll);
			break;
		case dliFailGetProc:
//			CServerIo::trace(1,"Missing function %s!",pdli->dlp.szProcName);
			break;
		case dliNoteEndProcessing: 
			break;
		default:
			break;
	}

	return NULL;
}
