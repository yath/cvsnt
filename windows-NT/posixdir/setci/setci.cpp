/*	cvsnt case sensitivity test program
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
// setci.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../cvsflt/cvsflt.h"

int _tmain(int argc, _TCHAR* argv[])
{
	PVOID Buf;
	DWORD BufLen;

	if(argc<2 || argc>3 || (wcscmp(argv[1],L"-l") && argc<3) || (wcscmp(argv[1],L"-l") && wcscmp(argv[1],L"-a") && wcscmp(argv[1],L"-d")))
	{
		fprintf(stderr,"Control directory case insensitivity\n\n");
		fprintf(stderr,"Usage: setci [-l] [-a dir] [-d dir]\n");
		fprintf(stderr,"\t-l\tList case insensitive directories\n");
		fprintf(stderr,"\t-a dir\tAdd to case insensitive directory list\n");
		fprintf(stderr,"\t-d dir\tRemove from case insensitive directory list\n");
		return -1;
	}

	if(!CvsOpenFilter())
	{
		fprintf(stderr,"Unable to open device driver (%08x)\n",GetLastError());
		return -1;
	}

	BufLen = MAX_PATH*4;
	Buf = malloc(BufLen);

	if(!wcscmp(argv[1],L"-a"))
	{
		DWORD fa = GetFileAttributes(argv[2]);
		if(fa==0xFFFFFFFF || !(fa&FILE_ATTRIBUTE_DIRECTORY))
		{
			fprintf(stderr, "%S is not a directory",argv[2]);
			return -1;
		}
		if(!CvsAddPosixDirectory(argv[2],FALSE))
		{
			fprintf(stderr,"Call to driver failed (%08x)\n",GetLastError());
			return -1;
		}
	} else if(!wcscmp(argv[1],L"-d"))
	{
		if(!CvsRemovePosixDirectory(argv[2],FALSE))
		{
			fprintf(stderr,"Call to driver failed (%08x)\n",GetLastError());
			return -1;
		}
	} else if(!wcscmp(argv[1],L"-l"))
	{
		TCHAR dir[MAX_PATH];
		unsigned short Count = 0;

		while(CvsEnumPosixDirectory(Count++,dir,sizeof(dir)/sizeof(dir[0])))
		{
			printf("%S\n",dir);
		} 
	}

	CvsCloseFilter();
	free(Buf);
	return 0;
}

