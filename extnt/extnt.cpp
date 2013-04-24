/* CVS legacy :ext: interface
    Copyright (C) 2004 Tony Hoyle and March-Hare Software Ltd

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
#include "stdafx.h"

#include <config.h>
#include <cvstools.h>

static void writethread(void *p);

static cvsroot root;
static int trace;

static int extnt_output(const char *str, size_t len)
{
	return printf("M %-*.*s",len,len,str);
}

static int extnt_error(const char *str, size_t len)
{
	return printf("E %-*.*s",len,len,str);
}

static int extnt_trace(int level, const char *str)
{
	OutputDebugString(str);
	OutputDebugString("\r\n");
	printf("%s\n",str);
	return 0;
}

void usage()
{
	fprintf(stderr,"Usage: extnt.exe hostname [-l username] [-p protocol] [-d directory] [command..]\n");
	fprintf(stderr,"\t-l username\tUsername to use (default current user).\n");
	fprintf(stderr,"\t-p protocol\tProtocol to use(default sspi).\n");
	fprintf(stderr,"\t-d directory\tDirectory part of root string.\n");
	fprintf(stderr,"\t-P password\tPassword to use.\n");
	fprintf(stderr,"\nUnspecified parameters are also read from [<hostname>] section in extnt.ini.\n");
	exit(-1);
}

/* Standard :ext: invokes with <hostname> -l <username> cvs server */
int main(int argc, char *argv[])
{
	CProtocolLibrary lib;
	char protocol[1024],hostname[1024],directory[1024],password[1024];
	char *section, *username = NULL, *repos = NULL, *prot = NULL, *passw = NULL;
	char buf[8000];

	if(argc<2)
		usage();

	section = argv[1];

	if(section[0]=='-')
		usage(); /* Options come *after* hostname */

	argv+=2;
	argc-=2;

	while(argc && argv[0][0]=='-')
	{
		if(argc<2)
			usage();
		int a=1;
		switch(argv[0][1])
		{
		case 'l':
			username=argv[1]; a++;
			break;
		case 'd':
			repos=argv[1]; a++;
			break;
		case 'p':
			prot=argv[1]; a++;
			break;
		case 'P':
			passw=argv[1]; a++;
			break;
		case 't':
			trace++;
			CServerIo::loglevel(trace);
			break;
		default:
			usage();
		}
		argv+=a;
		argc-=a;
	}

	// Any remaining arguments (usually 'cvs server') are ignored

	CServerIo::init(extnt_output,extnt_output,extnt_error,extnt_trace);

    WSADATA data;

    if (WSAStartup (MAKEWORD (1, 1), &data))
    {
		fprintf (stderr, "cvs: unable to initialize winsock\n");
		return -1;
    }

	setmode(0,O_BINARY);
	setmode(1,O_BINARY);
	setvbuf(stdin,NULL,_IONBF,0);
	setvbuf(stdout,NULL,_IONBF,0);

	lib.SetupServerInterface(&root,0);

	_snprintf(buf,sizeof(buf),"%s/extnt.ini",CGlobalSettings::GetConfigDirectory());

	if(prot)
		strcpy(protocol,prot);
	else
		GetPrivateProfileString(section,"protocol","sspi",protocol,sizeof(protocol),buf);
	GetPrivateProfileString(section,"hostname",section,hostname,sizeof(hostname),buf);
	if(repos)
		strcpy(directory,repos);
	else
		GetPrivateProfileString(section,"directory","",directory,sizeof(directory),buf);
	
	if(passw)
		strcpy(password,passw);
	else
		GetPrivateProfileString(section,"password","",password,sizeof(password),buf);

	root.method=protocol;
	root.hostname=hostname;
	root.directory=directory;
	root.username=username;
	root.password=password;

	if(!directory[0])
	{
		printf("error 0 [extnt] directory not specified in [%s] section of extnt.ini\n",section);
		return -1;
	}

	const struct protocol_interface *proto = lib.LoadProtocol(protocol);
	if(!proto)
	{
		printf("error 0 [extnt] Couldn't load procotol %s\n",protocol);
		return -1;
	}

	switch(proto->connect(proto, 0))
	{
	case CVSPROTO_SUCCESS: /* Connect succeeded */
		{
			char line[1024];
			int l = proto->read_data(proto,line,1024);
			line[l]='\0';
			if(!strcmp(line,"I HATE YOU\n"))
			{
				printf("error 0 [extnt] connect aborted: server %s rejeced access to %s\n",hostname,directory);
				return -1;
			}
			if(!strncmp(line,"cvs [",5))
			{
				printf("error 0 %s",line);
				return -1;
			}
			if(strcmp(line,"I LOVE YOU\n"))
			{
				printf("error 0 [extnt] Unknown response '%s' from protocol\n", line);
				return -1;
			}
			break;
		}
	case CVSPROTO_SUCCESS_NOPROTOCOL: /* Connect succeeded, don't wait for 'I LOVE YOU' response */
		break;
	case CVSPROTO_FAIL: /* Generic failure (errno set) */
		printf("error 0 [extnt] Connection failed\n");
		return -1;
	case CVSPROTO_BADPARMS: /* (Usually) wrong parameters from cvsroot string */
		printf("error 0 [extnt] Bad parameters\n");
		return -1;
	case CVSPROTO_AUTHFAIL: /* Authorization or login failed */
		printf("error 0 [extnt] Authentication failed\n");
		return -1;
	case CVSPROTO_NOTIMP: /* Not implemented */
		printf("error 0 [extnt] Not implemented\n");
		return -1;
	}

	_beginthread(writethread,0,(void*)proto);
	do
	{
		int len = proto->read_data(proto,buf,sizeof(buf));
		if(len==-1)
		{
			exit(-1); /* dead bidirectionally */
		}
		if(len)
			write(1,buf,len);
		Sleep(50);
	} while(1);

    return 0;
}

void writethread(void *p)
{
	const struct protocol_interface *proto = (const struct protocol_interface *)p;
	char buf[8000];

	do
	{
		int len = read(0,buf,sizeof(buf));
		/* stdin returns 0 when its input pipe goes dead */
		if(len<1)
		{
			exit(0); /* Dead bidirectionally */
		}
		proto->write_data(proto,buf,len);
		Sleep(50);
	} while(1);
}
