/*	cvsnt diagnostics
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
// cvsdiag.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../../version.h"

#include "../setuid/libsuid/suid.h"

char *get_os_version()
{
	static char osv[256];
	OSVERSIONINFO os = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&os);

	/* We will always be NT so no need to check that */
	if(os.dwMajorVersion>=5)
	{
		if(os.dwMinorVersion==0)
			strcpy(osv,"Windows 2000");
		else if(os.dwMinorVersion==1)
			strcpy(osv,"Windows XP");
		else
			strcpy(osv,"Windows 2003");
	}
	else
		strcpy(osv,"Windows NT");
	sprintf(osv+strlen(osv)," %d.%d.%d",os.dwMajorVersion,os.dwMinorVersion,os.dwBuildNumber);
	if(os.szCSDVersion[0])
		sprintf(osv+strlen(osv)," (%s)",os.szCSDVersion);
	return osv;
}

/* This function is a cut/paste from the internet.  There was no attribution on 
   the original source, but I suspect MSDN */
bool process_running(const char *process)
{
	typedef HANDLE (WINAPI *CREATESNAPSHOT)(DWORD dwFlags, DWORD dwProcID);
	typedef BOOL (WINAPI *PROCESS32FIRST)(HANDLE hSnapshot, LPPROCESSENTRY32 pEntry);
	typedef BOOL (WINAPI *PROCESS32NEXT)(HANDLE hSnapshot, LPPROCESSENTRY32 pEntry);
	
	HMODULE hKernel;
	HANDLE hSnapshot;
	PROCESSENTRY32 pe32;

	hKernel = GetModuleHandle("KERNEL32.DLL");
	if (!hKernel)
		return false;

	CREATESNAPSHOT pCreateToolhelp32Snapshot = (CREATESNAPSHOT)GetProcAddress(hKernel, "CreateToolhelp32Snapshot");
	PROCESS32FIRST pProcess32First = (PROCESS32FIRST)GetProcAddress(hKernel, "Process32First");
	PROCESS32NEXT pProcess32Next = (PROCESS32NEXT)GetProcAddress(hKernel, "Process32Next");

	if (!pCreateToolhelp32Snapshot || !pProcess32First || !pProcess32Next)	
		return false;

	if ((hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))==(void*)-1) 
		return false;

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!pProcess32First(hSnapshot, &pe32))
	{
		CloseHandle(hSnapshot);
		return false;
	}

	do
	{
		if(!stricmp(pe32.szExeFile,process))
		{
			CloseHandle(hSnapshot);
			return true;
		}
	} while(pProcess32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return false;
}

void print_found_files(FILE *output, const char *list)
{
	const char *p=list;
	bool detected=false;

	while(*p)
	{
		if(process_running(p))
		{
			fprintf(output,"%s ",p);
			detected=true;
		}
		p+=strlen(p)+1;
	}
	if(!detected)
		fprintf(output,"(none)");
	fprintf(output,"\n");
}

int is_readable(bool localsystem, const char *directory)
{
	HANDLE hFile;
	HANDLE hToken = NULL;

	if(localsystem)
	{
		// Doesn't work... fixme.
		if(SuidGetImpersonationToken("SYSTEM",NULL,LOGON32_LOGON_INTERACTIVE,&hToken))
		{
			return 2;
		}
		if(!ImpersonateLoggedOnUser(hToken))
		{
			CloseHandle(hToken);
			return 2;
		}
	}
	hFile=CreateFile(directory,GENERIC_READ,0,NULL,0,FILE_FLAG_BACKUP_SEMANTICS,NULL);
	if(hToken)
	{
		RevertToSelf();
		CloseHandle(hToken);
	}
	if(!hFile)
		return 0;
	CloseHandle(hFile);
	return 1;
}

int is_writable(bool localsystem, const char *directory)
{
	HANDLE hFile;
	HANDLE hToken = NULL;

	if(localsystem)
	{
		// Doesn't work... fixme.
		if(SuidGetImpersonationToken("SYSTEM",NULL,LOGON32_LOGON_INTERACTIVE,&hToken))
		{
			return 2;
		}
		if(!ImpersonateLoggedOnUser(hToken))
		{
			CloseHandle(hToken);
			return 2;
		}
	}
	hFile=CreateFile(directory,GENERIC_WRITE,0,NULL,0,FILE_FLAG_BACKUP_SEMANTICS,NULL);
	if(hToken)
	{
		RevertToSelf();
		CloseHandle(hToken);
	}
	if(!hFile)
		return 0;
	CloseHandle(hFile);
	return 1;
}

int get_reg_int(const char *key)
{
	HKEY hKey;
	DWORD dwVal;
	DWORD dwType,dwLen;

	if(RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\CVS\\Pserver",&hKey))
		return 0;

	dwLen=sizeof(dwVal);
	if(RegQueryValueEx(hKey,key,NULL,&dwType,(LPBYTE)&dwVal,&dwLen))
		return 0;

	return dwVal;
}

char *get_reg_string(const char *key)
{
	HKEY hKey;
	static char buf[4096];
	DWORD dwType,dwLen;

	if(RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\CVS\\Pserver",&hKey))
		return "(no key)";

	dwLen=sizeof(buf);
	if(RegQueryValueEx(hKey,key,NULL,&dwType,(LPBYTE)buf,&dwLen))
		return "(no value)";

	return buf;
}


bool protocol_installed(const char *protocol)
{
	char tmp[MAX_PATH];
	HMODULE hLibrary;

	sprintf(tmp,"%s\\protocols\\%s.dll",get_reg_string("InstallPath"),protocol);
	SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
	hLibrary=LoadLibrary(tmp);
	SetErrorMode(0);
	if(hLibrary)
	{
		FreeLibrary(hLibrary);
		return true;
	}
	return false;
}

bool service_installed(const char *service)
{
	SC_HANDLE hScm,hService;

	hScm=OpenSCManager(NULL,NULL,GENERIC_READ);
	if(!hScm)
		return false;
	hService=OpenService(hScm,service,SERVICE_QUERY_STATUS);
	if(!hService)
	{
		CloseServiceHandle(hScm);
		return false;
	}
	CloseServiceHandle(hScm);
	CloseServiceHandle(hService);
	return true;
}

void diag(FILE *output)
{
	char *yn[] = { "No", "Yes", "Unknown" };

	fprintf(output,"CVSNT Diagnostic output\n");
	fprintf(output,"-----------------------\n");
	fprintf(output,"\n");
	fprintf(output,"Server version: "CVSNT_PRODUCTVERSION_STRING"\n");
	fprintf(output,"OS Version: %s\n",get_os_version());
	fprintf(output,"\n");
	fprintf(output,"CVS Service installed: %s\n",service_installed("Cvsnt")?"Yes":"No");
	fprintf(output,"LockService installed: %s\n",service_installed("CvsLock")?"Yes":"No");
	fprintf(output,"\n");
	fprintf(output,":pserver: installed: %s\n",protocol_installed("pserver")?"Yes":"No");
	fprintf(output,":sserver: installed: %s\n",protocol_installed("sserver")?"Yes":"No");
	fprintf(output,":gserver: installed: %s\n",protocol_installed("gserver")?"Yes":"No");
	fprintf(output,":server: installed: %s\n",protocol_installed("server")?"Yes":"No");
	fprintf(output,":ssh: installed: %s\n",protocol_installed("ssh")?"Yes":"No");
	fprintf(output,":sspi: installed: %s\n",protocol_installed("sspi")?"Yes":"No");
	fprintf(output,":ext: installed: %s\n",protocol_installed("ext")?"Yes":"No");
	fprintf(output,"\n");
	fprintf(output,"Installation Path: %s\n",get_reg_string("InstallPath"));
	fprintf(output,"Repository 0 Path: %s\n",get_reg_string("Repository0"));
	fprintf(output,"Repository 0 Name: %s\n",get_reg_string("Repository0Name"));
	fprintf(output,"Repository 1 Path: %s\n",get_reg_string("Repository1"));
	fprintf(output,"Repository 1 Name: %s\n",get_reg_string("Repository1Name"));
	fprintf(output,"Repository 2 Path: %s\n",get_reg_string("Repository2"));
	fprintf(output,"Repository 2 Name: %s\n",get_reg_string("Repository2Name"));
	fprintf(output,"Repository 3 Path: %s\n",get_reg_string("Repository3"));
	fprintf(output,"Repository 3 Name: %s\n",get_reg_string("Repository3Name"));
	fprintf(output,"CVS Temp directory: %s\n",get_reg_string("TempDir"));
	fprintf(output,"CA Certificate File: %s\n",get_reg_string("CertificateFile"));
	fprintf(output,"Private Key File: %s\n",get_reg_string("PrivateKeyFile"));
	fprintf(output,"Local Users Only: %s\n",get_reg_int("DontUseDomain")?"Yes":"No");
	fprintf(output,"Default LockServer: %s\n",get_reg_string("LockServer"));
	fprintf(output,"Disable Reverse DNS: %s\n",get_reg_int("NoReverseDns")?"Yes":"No");
	fprintf(output,"Server Tracing: %s\n",get_reg_int("AllowTrace")?"Yes":"No");
	fprintf(output,"Case Sensitive: %s\n",get_reg_int("CaseSensitive")?"Yes":"No");
	fprintf(output,"Server listen port: %d\n",get_reg_int("PServerPort"));
	fprintf(output,"Compatibility (Non-cvsnt clients):\n");
	fprintf(output,"\tReport old CVS version: %s\n",get_reg_int("Compat0_OldVersion")?"Yes":"No");
	fprintf(output,"\tHide extended status: %s\n",get_reg_int("Compat0_HideStatus")?"Yes":"No");
	fprintf(output,"\tEmulate co -n bug: %s\n",get_reg_int("Compat0_OldCheckout")?"Yes":"No");
	fprintf(output,"\tIgnore client wrappers: %s\n",get_reg_int("Compat0_IgnoreWrappers")?"Yes":"No");
	fprintf(output,"Compatibility (CVSNT clients):\n");
	fprintf(output,"\tReport old CVS version: %s\n",get_reg_int("Compat1_OldVersion")?"Yes":"No");
	fprintf(output,"\tHide extended status: %s\n",get_reg_int("Compat1_HideStatus")?"Yes":"No");
	fprintf(output,"\tEmulate co -n bug: %s\n",get_reg_int("Compat1_OldCheckout")?"Yes":"No");
	fprintf(output,"\tIgnore client wrappers: %s\n",get_reg_int("Compat1_IgnoreWrappers")?"Yes":"No");
	fprintf(output,"Default domain: %s\n",get_reg_string("DefaultDomain"));
	fprintf(output,"Force run as user: %s\n",get_reg_string("RunAsUser"));
	fprintf(output,"\n");
	fprintf(output,"Temp dir readable by current user: %s\n",yn[is_readable(false,get_reg_string("TempDir"))]);
//	fprintf(output,"Temp dir readable by LocalSystem: %s\n",yn[is_readable(true,get_reg_string("TempDir"))]);
	fprintf(output,"Repository0 readable by current user: %s\n",yn[is_readable(false,get_reg_string("Repository0"))]);
//	fprintf(output,"Repository0 readable by LocalSystem: %s\n",yn[is_readable(true,get_reg_string("Repository0"))]);
	fprintf(output,"Temp dir writable by current user: %s\n",yn[is_writable(false,get_reg_string("TempDir"))]);
//	fprintf(output,"Temp dir writable by LocalSystem: %s\n",yn[is_writable(true,get_reg_string("TempDir"))]);
	fprintf(output,"\n");
	fprintf(output,"AV files detected:\n");
	print_found_files(output,"_AVP32.EXE\0_AVPCC.EXE\0_AVPM.EXE\0AVP32.EXE\0AVPCC.EXE\0AVPM.EXE\0"
							"N32SCANW.EXE\0NAVAPSVC.EXE\0NAVAPW32.EXE\0NAVLU32.EXE\0NAVRUNR.EXE\0NAVW32.EXE"
							"NAVWNT.EXE\0NOD32.EXE\0NPSSVC.EXE\0NRESQ32.EXE\0NSCHED32.EXE\0NSCHEDNT.EXE"
							"NSPLUGIN.EXE\0SCAN.EXE\0AVGSRV.EXE\0AVGSERV.EXE\0AVGCC32.EXE\0AVGCC.EXE\0"
							"AVGAMSVR.EXE\0AVGUPSVC.EXE\0NOD32KRN.EXE\0NOD32KUI.EXE\0");

	fprintf(output,"\n");
	WSADATA wsa = {0};
	if(WSAStartup(MAKEWORD(2,0),&wsa))
		fprintf(output,"Winsock intialisation failed!\n");
	else
	{
		fprintf(output,"Installed Winsock protocols:\n\n");
		DWORD dwSize=0;
		LPWSAPROTOCOL_INFO proto;
		WSAEnumProtocols(NULL,NULL,&dwSize);
		proto=(LPWSAPROTOCOL_INFO)malloc(dwSize);
		WSAEnumProtocols(NULL,proto,&dwSize);
		for(int n=0; n<(int)(dwSize/sizeof(proto[0])); n++)
		{
			if(!strncmp(proto[n].szProtocol,"MSAFD NetBIOS",13))
				continue; // Ignore netbios layers
			fprintf(output,"%d: %s\n",proto[n].dwCatalogEntryId,proto[n].szProtocol);
		}
		free(proto);
	}
}

int main(int argc, char* argv[])
{
	diag(stdout);
	return 0;
}

