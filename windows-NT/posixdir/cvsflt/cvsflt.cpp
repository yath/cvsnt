/*	cvsnt filter interface
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
#include "stdafx.h"
#include "cvsflt.h"
#include "cvsflt_pub.h"

static HANDLE g_hCvsFlt;

static int lstrncmpA(LPCSTR str1, LPCSTR str2, size_t len)
{
	while(len && *str1 && *str2 && (*str1)==(*str2))
	{
		len--;
		str1++;
		str2++;
	}
	return len && ((*str1)-(*str2));
}

static int lstrncmpW(LPCWSTR str1, LPCWSTR str2, size_t len)
{
	while(len && *str1 && *str2 && (*str1)==(*str2))
	{
		len--;
		str1++;
		str2++;
	}
	return len && ((*str1)-(*str2));
}

BOOL CvsOpenFilter()
{
	g_hCvsFlt = CreateFileW(CVSFLT_WIN32_DEVICE_NAME,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if(g_hCvsFlt==INVALID_HANDLE_VALUE)
	{
		g_hCvsFlt = NULL;
		return FALSE;
	}
	return TRUE;
}

BOOL CvsCloseFilter()
{
	if(g_hCvsFlt)
		CloseHandle(g_hCvsFlt);
	g_hCvsFlt = NULL;
	return TRUE;
}

BOOL CvsFilterIsOpen()
{
	return g_hCvsFlt?TRUE:FALSE;
}

static BOOL GetDosDeviceA(LPWSTR Dest, LPCSTR Source)
{
	return FALSE;
}

static BOOL GetDosDeviceW(LPWSTR Dest, LPCWSTR Source)
{
	LPWSTR p;
	BOOL bRet = TRUE;

	if(Source[1]==':' && IsCharAlpha(Source[0]))
	{
		lstrcpyW(Dest,L"\\DosDevices\\");
		lstrcatW(Dest,Source);
	}
	else if(lstrncmpW(Source,L"\\\\",2))
	{
		lstrcpy(Dest,L"\\Device\\LanmanRedirector");
		lstrcatW(Dest,Source+1);
	}
	else
		return FALSE;

	for(p=Dest;*p;p++)
		if(*p=='/') *p='\\';
	if(p>Dest && p[-1]=='\\')
		p[-1]='\0';

	/* QueryDosDevice gives us a symbolic name, which isn't really that useful at kernel level...
	   better to use the real kernel name... */
	wchar_t InBuf[MAX_PATH*2];
	wchar_t OutBuf[MAX_PATH*2];
	DWORD dwRet;
	struct _CVSFLT_GETKERNELNAME_IN *in = (struct _CVSFLT_GETKERNELNAME_IN*)InBuf;
	struct _CVSFLT_GETKERNELNAME_OUT *out = (struct _CVSFLT_GETKERNELNAME_OUT*)OutBuf;
	lstrcpyW(in->Directory,Dest);
	if(!DeviceIoControl(g_hCvsFlt,CVSFLT_GetKernelName,in,sizeof(InBuf),out,sizeof(OutBuf),&dwRet,NULL))
		bRet = FALSE;
	else
		lstrcpyW(Dest,out->Directory);

	return bRet;
}

BOOL CvsAddPosixDirectoryA(LPCSTR dir, BOOL temp)
{
	return FALSE;
}

BOOL CvsAddPosixDirectoryW(LPCWSTR dir, BOOL temp)
{
	DWORD dwRet,BufLen;
	LPVOID Buf;
	struct _CVSFLT_ADDPOSIXDIRECTORY_IN *Dir;
	BOOL bRet = TRUE;

	BufLen = MAX_PATH*4;
	Buf = (LPVOID)GlobalAlloc(GPTR,BufLen);
	Dir = (struct _CVSFLT_ADDPOSIXDIRECTORY_IN*)Buf;

	if(!GetDosDeviceW(Dir->Directory,dir))
		bRet = FALSE;
	else
	{
		Dir->Temporary=temp;
		if(!DeviceIoControl(g_hCvsFlt,CVSFLT_AddPosixDirectory,Buf,BufLen,Buf,BufLen,&dwRet,NULL))
			bRet = FALSE;
	}
	GlobalFree((HGLOBAL)Buf);
	return bRet;
}

BOOL CvsRemovePosixDirectoryA(LPCSTR dir, BOOL temp)
{
	return FALSE;
}

BOOL CvsRemovePosixDirectoryW(LPCWSTR dir, BOOL temp)
{
	DWORD dwRet,BufLen;
	LPVOID Buf;
	struct _CVSFLT_REMOVEPOSIXDIRECTORY_IN *Dir;
	BOOL bRet = TRUE;

	BufLen = MAX_PATH*4;
	Buf = (LPVOID)GlobalAlloc(GPTR,BufLen);
	Dir = (struct _CVSFLT_REMOVEPOSIXDIRECTORY_IN*)Buf;

	if(!GetDosDeviceW(Dir->Directory,dir))
		bRet = FALSE;
	else
	{
		Dir->Temporary=temp;
		if(!DeviceIoControl(g_hCvsFlt,CVSFLT_RemovePosixDirectory,Buf,BufLen,Buf,BufLen,&dwRet,NULL))
			bRet = FALSE;
	}
	GlobalFree((HGLOBAL)Buf);
	return bRet;
}

BOOL CvsEnumPosixDirectoryA(USHORT Index, LPSTR dir, DWORD cbLen)
{
	DWORD dwRet,BufLen;
	LPVOID Buf;
	struct _CVSFLT_GETPOSIXDIRECTORY_IN DirIn;
	struct _CVSFLT_GETPOSIXDIRECTORY_OUT *DirOut;
	BOOL bRet = TRUE;

	BufLen = cbLen + sizeof(struct _CVSFLT_GETPOSIXDIRECTORY_OUT);
	Buf = (LPVOID)GlobalAlloc(GPTR,BufLen);
	DirOut = (struct _CVSFLT_GETPOSIXDIRECTORY_OUT*)Buf;

	DirIn.Index = Index;

	if(!DeviceIoControl(g_hCvsFlt,CVSFLT_GetPosixDirectory,&DirIn,sizeof(DirIn),Buf,BufLen,&dwRet,NULL))
			bRet = FALSE;

	if(bRet)
		WideCharToMultiByte(CP_THREAD_ACP,0,DirOut->Directory,DirOut->Length/sizeof(DirOut->Directory[0]),dir,cbLen,NULL,NULL);

	GlobalFree((HGLOBAL)Buf);
	return bRet;
}

BOOL CvsEnumPosixDirectoryW(USHORT Index, LPWSTR dir, DWORD cbLen)
{
	DWORD dwRet,BufLen;
	LPVOID Buf;
	struct _CVSFLT_GETPOSIXDIRECTORY_IN DirIn;
	struct _CVSFLT_GETPOSIXDIRECTORY_OUT *DirOut;
	BOOL bRet = TRUE;

	BufLen = cbLen + sizeof(struct _CVSFLT_GETPOSIXDIRECTORY_OUT);
	Buf = (LPVOID)GlobalAlloc(GPTR,BufLen);
	DirOut = (struct _CVSFLT_GETPOSIXDIRECTORY_OUT*)Buf;

	DirIn.Index = Index;

	if(!DeviceIoControl(g_hCvsFlt,CVSFLT_GetPosixDirectory,&DirIn,sizeof(DirIn),Buf,BufLen,&dwRet,NULL))
			bRet = FALSE;

	if(bRet)
		lstrcpyn(dir,DirOut->Directory,cbLen);

	GlobalFree((HGLOBAL)Buf);
	return bRet;
}
