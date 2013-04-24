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
#ifndef MDNS_HOWL__H
#define MDNS_HOWL__H

#include "mdns.h"

/* Howl includes a config.h
  from the platform it was compliled on. Bleh. */

#ifdef HAVE_HOWL_H
#undef HAVE_CONFIG_H
#include <howl.h>
#endif

class CMdnsHelperHowl : public CMdnsHelperBase
{
public:
	CMdnsHelperHowl();
	virtual ~CMdnsHelperHowl();

	virtual int open();
	virtual int publish(const char *instance, const char *service, const char *location, int port, const char *text);
	virtual int step();
	virtual int browse(const char *service, MdnsBrowseCallback *callbacks, void *userdata);
	virtual int close();

protected:
	void *m_userdata;
	MdnsBrowseCallback *m_callbacks;
	time_t m_lastreply;
	sw_salt m_salt;
	sw_discovery m_discovery;

	static sw_result HOWL_API _service_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_discovery_publish_status	status,	sw_opaque extra);
	sw_result service_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_discovery_publish_status	status);
	static sw_result HOWL_API _browser_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_discovery_browse_status status, sw_uint32 interface_index,
				sw_const_string name, sw_const_string type,
				sw_const_string domain, sw_opaque_t extra);
	sw_result browser_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_discovery_browse_status status, sw_uint32 interface_index,
				sw_const_string name, sw_const_string type,
				sw_const_string domain);
	static sw_result HOWL_API _resolver_reply(sw_discovery discovery, sw_discovery_oid oid,
					sw_uint32 interface_index, sw_const_string name,
					sw_const_string type, sw_const_string domain,
					sw_ipv4_address address, sw_port port,
					sw_octets text_record, sw_uint32 text_record_len,
					sw_opaque_t extra);
	sw_result resolver_reply(sw_discovery discovery, sw_discovery_oid oid,
				sw_uint32 interface_index, sw_const_string name,
				sw_const_string type, sw_const_string domain,
				sw_ipv4_address address, sw_port port,
				sw_octets text_record, sw_uint32 text_record_len);
};

#endif
