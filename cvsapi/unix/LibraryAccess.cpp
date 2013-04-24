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
/* Unix specific */
#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ltdl.h>
#include <glob.h>

#include <config.h>
#include "../lib/api_system.h"

#include "../cvs_string.h"
#include "../FileAccess.h"
#include "../ServerIO.h"
#include "../LibraryAccess.h"

namespace
{
	static unsigned initcount;

	static int dlref()
	{
	if(!initcount++)
		lt_dlinit();
	}

	static int dlunref()
	{
	if(!--initcount)
		lt_dlexit();
	}
};

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
	if(directory && *directory)
		cvs::sprintf(fn,256,"%s/%s",directory,name);
	else
		fn = name;

	dlref();	
	m_lib = (void*)lt_dlopenext(fn.c_str());

	if(!m_lib)
	{
		CServerIo::trace(3,"LibraryAccess::Load failed for '%s', error = %s",fn.c_str(),strerror(errno));
		dlunref();
		return false;
	}

	return true;
}

bool CLibraryAccess::Unload()
{
	if(!m_lib)
		return true;

	lt_dlclose((lt_dlhandle)m_lib);
	dlunref();
	m_lib = NULL;
	return true;
}

void *CLibraryAccess::GetProc(const char *name)
{
	if(!m_lib)
		return NULL;

	return lt_dlsym((lt_dlhandle)m_lib,name);
}

void *CLibraryAccess::Detach()
{
	void *lib = m_lib;
	m_lib = NULL;
	return lib;
}
