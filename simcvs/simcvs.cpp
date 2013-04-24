/*  Call cvsnt at its installed location
    Copyright (C) 2004-5 Tony Hoyle and March-Hare Software Ltd

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

void DisplayString(LPCTSTR s)
{
	HANDLE hFile = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwUnused;

	if (GetFileType(hFile) != FILE_TYPE_CHAR)
		WriteFile(hFile, s, lstrlen(s) * sizeof (s[0]), & dwUnused, NULL);
	else
		WriteConsole(hFile, s, lstrlen(s), &dwUnused, NULL);
} 

int SimCvsStartup()
{
	HKEY hKeyLocal,hKeyGlobal;
	TCHAR Path[1024];
	DWORD dwLen,dwType,dwExit;
	LPWSTR lpszCmdParam;

	lpszCmdParam = GetCommandLine(); 

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"Software\\CVS\\PServer",0,KEY_QUERY_VALUE,&hKeyGlobal))
		hKeyGlobal = NULL;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Cvsnt\\PServer",0,KEY_QUERY_VALUE,&hKeyLocal))
		hKeyLocal = NULL;

	if(!hKeyGlobal && !hKeyGlobal)
	{
		DisplayString(L"Couldn't find cvs installation key");
		return -1;
	}

	dwLen=sizeof(Path);
	if(RegQueryValueEx(hKeyGlobal,L"InstallPath",NULL,&dwType,(LPBYTE)Path,&dwLen) &&
		RegQueryValueEx(hKeyLocal,L"InstallPath",NULL,&dwType,(LPBYTE)Path,&dwLen))
	{
		RegCloseKey(hKeyGlobal);
		RegCloseKey(hKeyLocal);
		DisplayString(L"Couldn't find cvs install path");
		return -1;
	}
	RegCloseKey(hKeyGlobal);
	RegCloseKey(hKeyLocal);
	
	lstrcat(Path,L"\\cvs.exe");

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	if(!CreateProcess(Path,lpszCmdParam,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
	{
		DisplayString(L"Couldn't run cvs process");
		return -1;
	}
	WaitForSingleObject(pi.hProcess,INFINITE);
	GetExitCodeProcess(pi.hProcess,&dwExit);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (int)dwExit;
}
