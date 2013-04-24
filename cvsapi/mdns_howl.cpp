/*
	CVSNT mdns helpers - howl
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <delayimp.h>
#endif

#include <config.h>
#include "lib/api_system.h"
#include "cvs_string.h"
#include "ServerIO.h"
#include "mdns_howl.h"

sw_result CMdnsHelperHowl::_service_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_discovery_publish_status	status,	sw_opaque extra)
{
	return ((CMdnsHelperHowl*)extra)->service_reply(discovery,oid,status);
}

sw_result CMdnsHelperHowl::service_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_discovery_publish_status	status)
{
	static const sw_string status_text[] =
	{
		"Zeroconf data published.",
		"Zeroconf server stopped.",
		"This server name is already in use.  Zeroconf data not published.",
		"Invalid Zeroconf data"
	};

	fprintf(stderr, "%s\n", status_text[status]);
	return SW_OKAY;
}

sw_result CMdnsHelperHowl::_browser_reply(sw_discovery discovery, sw_discovery_oid oid,
			  sw_discovery_browse_status status, sw_uint32 interface_index,
			  sw_const_string name,	sw_const_string	type,
			  sw_const_string domain, sw_opaque_t extra)
{
	return ((CMdnsHelperHowl*)extra)->browser_reply(discovery,oid,status,interface_index,
											name,type,domain);
}

sw_result CMdnsHelperHowl::browser_reply(sw_discovery discovery, sw_discovery_oid oid,
			  sw_discovery_browse_status status, sw_uint32 interface_index,
			  sw_const_string name,	sw_const_string	type,
			  sw_const_string domain)
{
	sw_discovery_resolve_id rid;

	switch (status)
	{
		case SW_DISCOVERY_BROWSE_INVALID:
		case SW_DISCOVERY_BROWSE_ADD_DOMAIN:
		case SW_DISCOVERY_BROWSE_ADD_DEFAULT_DOMAIN:
		case SW_DISCOVERY_BROWSE_REMOVE_DOMAIN:
		case SW_DISCOVERY_BROWSE_REMOVE_SERVICE:
		case SW_DISCOVERY_BROWSE_RESOLVED:
			break;
		case SW_DISCOVERY_BROWSE_ADD_SERVICE:
		{
			if (sw_discovery_resolve(discovery, interface_index, name, type, domain, _resolver_reply, this, &rid) != SW_OKAY)
			{
				fprintf(stderr, "resolve failed\n");
			}
		}
		break;

	}

	return SW_OKAY;
}

sw_result CMdnsHelperHowl::_resolver_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_uint32 interface_index, sw_const_string name,
				sw_const_string type, sw_const_string domain,
				sw_ipv4_address address, sw_port port,
				sw_octets text_record, sw_uint32 text_record_len,
				sw_opaque_t extra)
{
	return ((CMdnsHelperHowl*)extra)->resolver_reply(discovery,oid,interface_index,name,
									type,domain,address,port,text_record,text_record_len);
}

sw_result CMdnsHelperHowl::resolver_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_uint32 interface_index, sw_const_string name,
				sw_const_string type, sw_const_string domain,
				sw_ipv4_address address, sw_port port,
				sw_octets text_record, sw_uint32 text_record_len)
{
	sw_text_record_iterator	it;
	char key[SW_TEXT_RECORD_MAX_LEN];
	sw_uint8 oval[SW_TEXT_RECORD_MAX_LEN];
	sw_uint32 oval_len;
	sw_result err = SW_OKAY;
	cvs::string txt,tmp;

	sw_discovery_cancel(discovery, oid);
	
	if ((text_record_len > 0) && (text_record) && (*text_record != '\0'))
	{
		err = sw_text_record_iterator_init(&it, text_record, text_record_len);

		while (sw_text_record_iterator_next(it, key, oval, &oval_len) == SW_OKAY)
		{
			cvs::sprintf(tmp,80,"%s=%s\n", key, oval);
			txt+=tmp;
		}

		err = sw_text_record_iterator_fina(it);
	}

	cvs::string fullname;

	char target[32];

	sw_ipv4_address_name(address,target,sizeof(target));


	cvs::sprintf(fullname,80,"%s.%s%s",name,type,domain);
	if(fullname.length()>0 && fullname[fullname.length()-1]=='.')
		fullname.resize(fullname.size()-1);

	if(m_callbacks->srv_fn)
		m_callbacks->srv_fn(fullname.c_str(),port,target,m_userdata);
	if(m_callbacks->txt_fn)
		m_callbacks->txt_fn(fullname.c_str(),txt.c_str(),m_userdata);
	if(m_callbacks->ipv4_fn)
		m_callbacks->ipv4_fn(target,(unsigned char*)&address.m_addr,m_userdata);

	return err;
}


CMdnsHelperHowl::CMdnsHelperHowl()
{
	m_discovery = NULL;
	m_salt = NULL;
}

CMdnsHelperHowl::~CMdnsHelperHowl()
{
	close();
}

int CMdnsHelperHowl::open()
{
#ifdef _WIN32
	__try
	{
		if (FAILED(__HrLoadAllImportsForDll("howl.dll")))
		{
			CServerIo::error("Couldn't find howl.dll - failing mdns initialisation\n");
			return -1;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		CServerIo::error("Couldn't find howl.dll - failing mdns initialisation\n");
		return -1;
	}
#endif
	if (sw_discovery_init(&m_discovery) != SW_OKAY)
	{
		fprintf(stderr, "sw_discovery_init() failed\n");
		return -1;
	}
	return 0;
}

int CMdnsHelperHowl::publish(const char *instance, const char *service, const char *location, int port, const char *text)
{
	sw_result						result;
	sw_discovery_publish_id	id;
	sw_text_record txt;

	if(text && !*text)
		text=NULL;

	if(text)
	{
		sw_text_record_init(&txt);
		sw_text_record_add_string(txt, text);
	}

	char tmp[256];
	strncpy(tmp,service,sizeof(tmp));
	char *p=tmp+strlen(tmp)-1;
	if(strlen(tmp)>0 && *p=='.') *(p--)='\0';
	if(strlen(tmp)>6 && !strcmp(p-5,".local")) *(p-5)='\0';

	service = tmp;

	if ((result = sw_discovery_publish(m_discovery, 0, instance, service, NULL, location, port, text?sw_text_record_bytes(txt):NULL, text?sw_text_record_len(txt):0, _service_reply, this, &id)) != SW_OKAY)
	{
		fprintf(stderr, "publish failed: %d\n", result);
		return -1;
	}

	sw_discovery_salt(m_discovery,&m_salt);

	if(text && *text)
		sw_text_record_fina(txt);

	return 0;
}

int CMdnsHelperHowl::step()
{
	sw_ulong msecs = 500;
	sw_salt_step(m_salt,&msecs);
	return 0;
}

int CMdnsHelperHowl::browse(const char *service, MdnsBrowseCallback *callbacks, void *userdata)
{
	sw_discovery_oid oid;
	m_userdata = userdata;
	m_callbacks = callbacks;

	if(sw_discovery_browse(m_discovery, 0, service, NULL, _browser_reply, this, &oid))
	{
		CServerIo::trace(3,"sw_discovery_browse() failed.\n");
		return -1;
	}

	sw_discovery_salt(m_discovery,&m_salt);

	time_t t;
	time(&m_lastreply);
	do
	{
		step();
		time(&t);
	} while(t<m_lastreply+2);
	return 0;
}

int CMdnsHelperHowl::close()
{
	if(m_discovery)
		sw_discovery_fina(m_discovery);
	m_salt = NULL;
	m_discovery = NULL;
	return 0;
}

extern "C" CMdnsHelperBase *MdnsHelperHowl_Alloc()
{
	return new CMdnsHelperHowl();
}
