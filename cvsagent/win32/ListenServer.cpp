/*	cvsnt agent
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
#include "StdAfx.h"
#include "ListenServer.h"

CListenServer::CListenServer()
{
	WSADATA wsa={sizeof(WSADATA)};
	WSAStartup(MAKEWORD(2,0),&wsa);
}

CListenServer::~CListenServer()
{
}

bool CListenServer::Listen(LPCSTR szPort)
{
	int err;
    addrinfo *pAddrInfo;
	std::vector<SOCKET> sockets;

	addrinfo hint = {0};
	hint.ai_family=PF_UNSPEC;
	hint.ai_socktype=SOCK_STREAM;
	hint.ai_protocol=IPPROTO_TCP;
	hint.ai_flags=0;
	pAddrInfo=NULL;
	err=getaddrinfo(NULL,szPort,&hint,&pAddrInfo);
	if(err)
	{
#ifdef _DEBUG
		std::wstring szErr = gai_strerror(err);
#endif
		return false;
	}

	addrinfo* ai;
	for(ai=pAddrInfo;ai;ai=ai->ai_next)
	{
		SOCKET s = WSASocket(ai->ai_family,ai->ai_socktype,ai->ai_protocol,NULL,0,0);
		if(s!=-1 && !bind(s,ai->ai_addr,(int)ai->ai_addrlen))
		{
			if(listen(s,50)==SOCKET_ERROR)
			{
				return false;
			}
			sockets.push_back(s);
		}
		else
			closesocket(s);
	}
	freeaddrinfo(pAddrInfo);

	if(!sockets.size())
		return false;

	do
	{
		fd_set rfd;
		sockaddr_storage sin;
		size_t n;
		int addrlen;
 		size_t maxdesc = 0;

		FD_ZERO(&rfd);
		for(n=0; n<sockets.size(); n++)
		{
			FD_SET(sockets[n],&rfd);
			if(((int)sockets[n])>maxdesc)
			  maxdesc=sockets[n];
		}
		struct timeval tv = { 5, 0 }; // 5 seconds max wait
		int sel=select((int)maxdesc+1,&rfd,NULL,NULL,&tv);
		if(sel==SOCKET_ERROR) break; // Error on socket
		size_t size = sockets.size();
		for(n=0; n<size; n++)
		{
			if(FD_ISSET(sockets[n],&rfd))
			{
				addrlen=sizeof(sin);
				SOCKET conn = accept(sockets[n],(struct sockaddr*)&sin,&addrlen);
				if(conn!=SOCKET_ERROR)
				{
					DoServer(conn);
					closesocket(conn);
				}
			}
		}
	} while(true);
	return true;
}

bool CListenServer::DoServer(SOCKET s)
{
	char buffer[BUFSIZ];
	int len;

	len=recv(s,buffer,sizeof(buffer),0);
	if(len<=0)
	{
#ifdef _DEBUG
		std::wstring szErr = gai_strerror(WSAGetLastError());
#endif
		return false;
	}
	buffer[len]='\0';
	// lookup the password, etc.
	if(buffer[0]=='*')
	{
		AfxGetApp()->m_pMainWnd->SendMessage(PWD_MESSAGE_DELETE,(WPARAM)&buffer[1],(LPARAM)NULL);
	}
	else if(AfxGetApp()->m_pMainWnd->SendMessage(PWD_MESSAGE,(WPARAM)buffer,(LPARAM)&len))
	{
			send(s,buffer,len+1,0);
	}
	else
	{
		buffer[0]=-1;
		send(s,buffer,1,0);
	}

	return true;
}

