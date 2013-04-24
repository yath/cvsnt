/*  Call Control Panel at its installed location with modified path
    Copyright (C) 2005 Tony Hoyle and March-Hare Software Ltd

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <cpl.h>

extern "C" BOOL WINAPI _DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}

extern "C" LONG WINAPI CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	static LONG (WINAPI* pCPlApplet)(HWND, UINT, LPARAM, LPARAM);
	static HMODULE hCpl;
	static CPLINFO info = {0};
	long ret;

	if(uMsg == CPL_INIT)
	{
		HKEY hKey;
		static TCHAR szCvs[1024],szPath[1024];
		DWORD dwLen,dwType;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\CVS\\Pserver",0,KEY_QUERY_VALUE,&hKey))
			return 0;

		dwLen=sizeof(szCvs);
		if(RegQueryValueEx(hKey,L"InstallPath",NULL,&dwType,(LPBYTE)szCvs,&dwLen))
		{
			RegCloseKey(hKey);
			return 0;
		}
		RegCloseKey(hKey);
		
		// LOAD_WITH_ALTERED_SEARCH_PATH is buggy, and SetDllDirectory is XP only
		GetCurrentDirectory(sizeof(szPath),szPath);
		SetCurrentDirectory(szCvs);
		hCpl = LoadLibrary(L"cvsnt.cpl");
		SetCurrentDirectory(szPath);

		if(!hCpl)
			return 0;

		pCPlApplet=(LONG (WINAPI*)(HWND, UINT, LPARAM, LPARAM))GetProcAddress(hCpl,"CPlApplet");

		if(!pCPlApplet)
			return 0;
	}

	if(uMsg == CPL_NEWINQUIRE && (info.idIcon || info.idInfo || info.idName))
	{
		LPNEWCPLINFO cpl = (LPNEWCPLINFO)lParam2;

		cpl->dwSize = sizeof(NEWCPLINFO);
		cpl->lData = info.lData;
		cpl->hIcon = LoadIcon(hCpl,MAKEINTRESOURCE(info.idIcon));
		LoadString(hCpl,info.idInfo,cpl->szInfo,sizeof(cpl->szInfo)/sizeof(cpl->szInfo[0]));
		LoadString(hCpl,info.idName,cpl->szName,sizeof(cpl->szName)/sizeof(cpl->szName[0]));

		ret = 0;
	}
	else
	{
		ret = pCPlApplet(hwndCPl,uMsg,lParam1,lParam2);

		if(uMsg == CPL_INQUIRE && !ret)
		{
			LPCPLINFO cpl = (LPCPLINFO)lParam2;
			info = *cpl;
			cpl->idIcon = CPL_DYNAMIC_RES;
			cpl->idInfo = CPL_DYNAMIC_RES;
			cpl->idName = CPL_DYNAMIC_RES;
		}

		if(uMsg == CPL_EXIT)
		{
			FreeLibrary(hCpl);
		}
	}
	return ret;
}
