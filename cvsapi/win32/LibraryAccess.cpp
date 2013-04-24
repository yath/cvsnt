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

#include <config.h>
#include "../lib/api_system.h"

#include "../cvs_string.h"
#include "../FileAccess.h"
#include "../ServerIO.h"
#include "../LibraryAccess.h"

CLibraryAccess::CLibraryAccess(const void *lib /*= NULL*/)
{
	m_lib = (void*)lib;
}

CLibraryAccess::~CLibraryAccess()
{
	Unload();
}

bool CLibraryAccess::Load(const char *name, const char *directory)
{
	if(m_lib)
		Unload();

	cvs::filename fn;
	if(directory)
		cvs::sprintf(fn,256,"%s/%s",directory,name);
	else
		fn = name;
	CServerIo::trace(3,"CLibraryAccess::Load loading %s",fn.c_str());
	UINT uMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
	m_lib = (void*)LoadLibrary(CFileAccess::Win32Wide(fn.c_str()));
	SetErrorMode(uMode);

	if(!m_lib)
	{
		CServerIo::trace(3,"LibraryAccess::Load failed for '%s', Win32 error %08x",fn.c_str(),GetLastError());
		return false;
	}

	return true;
}

bool CLibraryAccess::Unload()
{
	if(!m_lib)
		return true;

	FreeLibrary((HMODULE)m_lib);
	m_lib = NULL;
	return true;
}

void *CLibraryAccess::GetProc(const char *name)
{
	if(!m_lib)
		return NULL;

	return (void*)GetProcAddress((HMODULE)m_lib,name);
}

void *CLibraryAccess::Detach()
{
	void *lib = m_lib;
	m_lib = NULL;
	return lib;
}

