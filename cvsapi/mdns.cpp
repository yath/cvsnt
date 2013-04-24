/*
	CVSNT mdns helpers
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
#include <config.h>
#include "lib/api_system.h"
#include "cvs_string.h"
#include "ServerIO.h"
#include "mdns.h"
#include "LibraryAccess.h"

#ifdef _WIN32
#define STATIC_MDNS /* We can use late binding in Win32 to be more efficient */
#else
#undef STATIC_MDNS
#endif

extern "C" CMdnsHelperBase *MdnsHelperMini_Alloc();
extern "C" CMdnsHelperBase *MdnsHelperHowl_Alloc();
extern "C" CMdnsHelperBase *MdnsHelperApple_Alloc();

CMdnsHelperBase* CMdnsHelperBase::Alloc(mdnsType type, const char *dir)
{
#ifdef STATIC_MDNS
	switch(type)
	{
	case mdnsMini:
		CServerIo::trace(3,"Loading miniMdns");
		return MdnsHelperMini_Alloc();
#ifdef HAVE_HOWL_H
	case mdnsHowl:
		CServerIo::trace(3,"Loading Howl mdns");
		return MdnsHelperHowl_Alloc();
#endif
#ifdef HAVE_DNS_SD_H
	case mdnsApple:
		CServerIo::trace(3,"Loading Apple mdns");
		return MdnsHelperApple_Alloc();
#endif
	default:
		return NULL;
	}
#else /* Not STATIC_MDNS */
	CLibraryAccess la;
	CMdnsHelperBase* (*pNewMdnsHelper)() = NULL;

	switch(type)
	{
	case mdnsMini:
		CServerIo::trace(3,"Loading miniMdns");
		if(!la.Load("mini"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewMdnsHelper = (CMdnsHelperBase*(*)())la.GetProc("MdnsHelperMini_Alloc");
		break;
	case mdnsHowl:
		CServerIo::trace(3,"Loading Howl mdns");
		if(!la.Load("howl"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewMdnsHelper = (CMdnsHelperBase*(*)())la.GetProc("MdnsHelperHowl_Alloc");
		break;
	case mdnsApple:
		CServerIo::trace(3,"Loading Apple mdns");
		if(!la.Load("apple"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewMdnsHelper = (CMdnsHelperBase*(*)())la.GetProc("MdnsHelperApple_Alloc");
		break;
	default:
		return NULL;
	}
	if(!pNewMdnsHelper)
		return NULL;
	CMdnsHelperBase *mdns = pNewMdnsHelper();
	la.Detach(); // Add a reference so the library never unloads
	return mdns;
#endif
}
