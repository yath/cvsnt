/*
	CVSNT Audit trigger handler
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
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define MODULE audit

#include <ctype.h>
#include <cvstools.h>
#include "../version.h"

#ifdef _WIN32
HMODULE g_hInst;

BOOL CALLBACK DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	g_hInst = hModule;
	return TRUE;
}

#include "audit_resource.h"

static int win32config(const struct plugin_interface *ui, void *wnd);
#endif

struct diffstore_t
{
	diffstore_t() { added=removed=0; }
	unsigned long added;
	unsigned long removed;
	cvs::string diff;
};

CSqlConnection *g_pDb;
cvs::string g_error;
char g_szPrefix[256];
bool g_AuditLogSessions;
bool g_AuditLogCommits;
bool g_AuditLogDiffs;
bool g_AuditLogTags;
bool g_AuditLogHistory;
unsigned long g_nSessionId;
std::map<cvs::filename,diffstore_t> g_diffStore;

#define NULLSTR(s) ((s)?(s):"")

static CSqlConnection* Connect(int nType, const char *name, const char *host, const char *user, const char *password, cvs::string& error)
{
	CSqlConnection *pDb;

	switch(nType)
	{
	case 0:
		pDb = CSqlConnection::Alloc(sqtMysql,CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDDatabase));
		break;
	case 1:
		pDb = CSqlConnection::Alloc(sqtSqlite,CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDDatabase));
		break;
	case 2:
		pDb = CSqlConnection::Alloc(sqtPostgres,CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDDatabase));
		break;
	case 3:
		pDb = CSqlConnection::Alloc(sqtOdbc,CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDDatabase));
		break;
	case 4:
		pDb = CSqlConnection::Alloc(sqtMssql,CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDDatabase));
		break;
	case 5:
		pDb = CSqlConnection::Alloc(sqtDb2,CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDDatabase));
		break;
	default:
		pDb = NULL;
	};

	if(!pDb)
	{
		error = "Couldn't initialise database engine.";
		return NULL;
	}

	if(!pDb->Open(host,name,user,password))
	{
		cvs::sprintf(error,80,"Open failed: %s",pDb->ErrorString());
		delete pDb;
		return NULL; 
	}

	return pDb;
}

int init(const struct trigger_interface_t* cb, const char *command, const char *date, const char *hostname, const char *username, const char *virtual_repository, const char *physical_repository, const char *sessionid, const char *editor, int count_uservar, const char **uservar, const char **userval, const char *client_version, const char *character_set)
{
	int nType;
	char value[256],name[256],user[256],password[256],host[256];
	int val = 0;

	if(!CGlobalSettings::GetGlobalValue("cvsnt","Plugins","AuditTrigger",value,sizeof(value)))
		val = atoi(value);
	if(!val)
	{
		CServerIo::trace(3,"Audit trigger not enabled.");
		return -1;
	}

	g_diffStore.clear();

	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseType",value,sizeof(value)))
		nType = atoi(value);
	else
		nType = 1;

	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseName",name,sizeof(name)))
	{
		CServerIo::trace(3,"Audit plugin: Database name not set");
		g_error = "Database name not set";
		g_pDb = NULL;
		return 0;
	}
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabasePrefix",g_szPrefix,sizeof(g_szPrefix)))
		g_szPrefix[0]='\0';
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseHost",host,sizeof(host)))
		strcpy(host,"localhost");
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseUsername",user,sizeof(user)))
		user[0]='\0';
	if(CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabasePassword",password,sizeof(password)))
		password[0]='\0';

	g_error = "";
	g_pDb = Connect(nType,name,host,user,password,g_error);

	if(!g_pDb)
	{
		CServerIo::trace(3,"Audit trigger database connection failed: %s",g_error.c_str());
		// Fail in precommand
		return 0;
	}

	g_AuditLogSessions = false;
	g_AuditLogCommits = false;
	g_AuditLogDiffs = false;
	g_AuditLogTags = false;
	g_AuditLogHistory = false;

	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogSessions",value,sizeof(value)))
		g_AuditLogSessions=atoi(value)?true:false;
	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogCommits",value,sizeof(value)))
		g_AuditLogCommits=atoi(value)?true:false;
	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogDiffs",value,sizeof(value)))
		g_AuditLogDiffs=atoi(value)?true:false;
	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogTags",value,sizeof(value)))
		g_AuditLogTags=atoi(value)?true:false;
	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogHistory",value,sizeof(value)))
		g_AuditLogHistory=atoi(value)?true:false;

	g_nSessionId = 0;
	if(g_AuditLogSessions)
	{
		time_t d = get_date((char*)date,NULL);
		char dt[64];
		cvs::string tbl;
		cvs::sprintf(tbl,80,"%sSessionLog",g_szPrefix);
		strftime(dt,sizeof(dt),"%Y-%m-%d %H:%M:%S",localtime(&d));
		g_pDb->Execute("Insert Into %s (Command, Date, Hostname, Username, SessionId, VirtRepos, PhysRepos, Client) Values ('%s','%s','%s','%s','%s','%s','%s','%s')",
			tbl.c_str(),NULLSTR(command),dt,NULLSTR(hostname),NULLSTR(username),NULLSTR(sessionid),NULLSTR(virtual_repository),NULLSTR(physical_repository),NULLSTR(client_version));
		if(g_pDb->Error())
		{
			CServerIo::error("audit_trigger error (session): %s\n",g_pDb->ErrorString());
			delete g_pDb;
			g_pDb = NULL;
			return 0;
		}
		else
			g_nSessionId=g_pDb->GetInsertIdentity(tbl.c_str());
	}

	return 0;
}

int close(const struct trigger_interface_t* cb)
{
	if(g_pDb)
		delete g_pDb;
	return 0;
}

int pretag(const struct trigger_interface_t* cb, const char *message, const char *directory, int name_list_count, const char **name_list, const char **version_list, char tag_type, const char *action, const char *tag)
{
	if(g_AuditLogTags)
	{
		for(int n=0; n<name_list_count; n++)
		{
			const char *filename = name_list[n];
			const char *rev = version_list[n];

			g_pDb->Bind(0,message?message:"");
			if(g_AuditLogSessions)
				g_pDb->Execute("Insert Into %sTagLog (SessionId, Directory, Filename, Tag, Revision, Message, Action, Type) Values (%lu, '%s','%s','%s','%s',?,'%s','%c')",
					g_szPrefix,g_nSessionId,NULLSTR(directory),NULLSTR(filename),NULLSTR(tag),NULLSTR(rev),NULLSTR(action),tag_type);
			else
				g_pDb->Execute("Insert Into %sTagLog (Directory, Filename, Tag, Revision, Message, Action, Type) Values (%lu, '%s','%s','%s','%s',?,'%s','%c')",
					g_szPrefix,NULLSTR(directory),NULLSTR(filename),NULLSTR(tag),NULLSTR(rev),NULLSTR(action),tag_type);
			if(g_pDb->Error())
			{
				CServerIo::error("audit_trigger error (pretag): %s\n",g_pDb->ErrorString());
				return -1;
			}
		}
	}
	return 0;
}

int verifymsg(const struct trigger_interface_t* cb, const char *directory, const char *filename)
{
	return 0;
}

int loginfo(const struct trigger_interface_t* cb, const char *message, const char *status, const char *directory, int change_list_count, change_info_t *change_list)
{
	if(g_AuditLogCommits)
	{
		for(int n=0; n<change_list_count; n++)
		{
			const char *diff = g_diffStore[change_list[n].filename].diff.c_str();
			unsigned long added = g_diffStore[change_list[n].filename].added;
			unsigned long removed = g_diffStore[change_list[n].filename].removed;

			g_pDb->Bind(0,message?message:"");
			g_pDb->Bind(1,diff);
			if(g_AuditLogSessions)
				g_pDb->Execute("Insert Into %sCommitLog (SessionId, Directory, Message, Type, Filename, Tag, BugId, OldRev, NewRev, Added, Removed, Diff) Values (%lu, '%s', ? ,'%c','%s','%s','%s','%s','%s',%lu, %lu, ? )",
					g_szPrefix,g_nSessionId,NULLSTR(directory),change_list[n].type,NULLSTR(change_list[n].filename),NULLSTR(change_list[n].tag),NULLSTR(change_list[n].bugid),NULLSTR(change_list[n].rev_old),NULLSTR(change_list[n].rev_new),added,removed);
			else
				g_pDb->Execute("Insert Into %sCommitLog (Directory, Message, Type, Filename, Tag, BugId, OldRev, NewRev, Added, Removed, Diff) Values (%lu, ? ,'%s','%c','%s','%s','%s','%s','%s',%lu, %lu, ? )",
					g_szPrefix,NULLSTR(directory),change_list[n].type,NULLSTR(change_list[n].filename),NULLSTR(change_list[n].tag),NULLSTR(change_list[n].bugid),NULLSTR(change_list[n].rev_old),NULLSTR(change_list[n].rev_new),added,removed);
			if(g_pDb->Error())
			{
				CServerIo::error("audit_trigger error (loginfo): %s\n",g_pDb->ErrorString());
				return -1;
			}
		}
	}
	g_diffStore.clear();
	return 0;
}

int history(const struct trigger_interface_t* cb, char type, const char *workdir, const char *revs, const char *name, const char *bugid, const char *message)
{
	if(g_AuditLogHistory)
	{
		g_pDb->Bind(0,message?message:"");
		if(g_AuditLogSessions)
			g_pDb->Execute("Insert Into %sHistoryLog (SessionId, Type, Workdir, Revs, Name, BugId, Message) Values (%lu, '%c','%s','%s','%s','%s', ? )",
				g_szPrefix,g_nSessionId,type,NULLSTR(workdir),NULLSTR(revs),NULLSTR(name),NULLSTR(bugid));
		else
			g_pDb->Execute("Insert Into %sHistoryLog (Type, Workdir, Revs, Name, BugId, Message) Values ('%c','%s','%s','%s','%s', ? )",
				g_szPrefix,type,NULLSTR(workdir),NULLSTR(revs),NULLSTR(name),NULLSTR(bugid));
		if(g_pDb->Error())
		{
			CServerIo::error("audit_trigger error (history): %s\n",g_pDb->ErrorString());
			return -1;
		}
	}
	return 0;
}

int notify(const struct trigger_interface_t* cb, const char *message, const char *bugid, const char *directory, const char *notify_user, const char *tag, const char *type, const char *file)
{
	return 0;
}

int precommit(const struct trigger_interface_t* cb, int name_list_count, const char **name_list, const char *message, const char *directory)
{
	return 0;
}

int postcommit(const struct trigger_interface_t* cb, const char *directory)
{
	return 0;
}

int precommand(const struct trigger_interface_t* cb, int argc, const char **argv)
{
	if(!g_pDb)
	{
		CServerIo::error("Audit trigger initialiasation failed: %s\n",g_error.c_str());
		return -1;
	}
	return 0;
}

int postcommand(const struct trigger_interface_t* cb, const char *directory)
{
	return 0;
}

int premodule(const struct trigger_interface_t* cb, const char *module)
{
	return 0;
}

int postmodule(const struct trigger_interface_t* cb, const char *module)
{
	return 0;
}

int get_template(const struct trigger_interface_t *cb, const char *directory, const char **template_ptr)
{
	return 0;
}

int parse_keyword(const struct trigger_interface_t *cb, const char *keyword,const char *directory,const char *file,const char *branch,const char *author,const char *printable_date,const char *rcs_date,const char *locker,const char *state,const char *version,const char *name,const char *bugid, const char *commitid, const property_info *props, size_t numprops, const char **value)
{
	return 0;
}

int prercsdiff(const struct trigger_interface_t *cb, const char *file, const char *directory, const char *oldfile, const char *newfile, const char *type, const char *options, const char *oldversion, const char *newversion, unsigned long added, unsigned long removed)
{
	g_diffStore[file].added=added;
	g_diffStore[file].removed=removed;

	// return 0 - no diff (rcsdiff not called)
	// return 1 - Unified diff
	if(g_AuditLogDiffs && (added || removed) && (!options || !strchr(options,'b')))
		return 1;
	return 0;
}

int rcsdiff(const struct trigger_interface_t *cb, const char *file, const char *directory, const char *oldfile, const char *newfile, const char *diff, size_t difflen, const char *type, const char *options, const char *oldversion, const char *newversion, unsigned long added, unsigned long removed)
{
	g_diffStore[file].added=added;
	g_diffStore[file].removed=removed;
	g_diffStore[file].diff=diff;
	return 0;
}

static int init(const struct plugin_interface *plugin);
static int destroy(const struct plugin_interface *plugin);
static void *get_interface(const struct plugin_interface *plugin, unsigned interface_type, void *param);

static trigger_interface callbacks =
{
	{
		PLUGIN_INTERFACE_VERSION,
		"Repository auditing extension",CVSNT_PRODUCTVERSION_STRING,"AuditTrigger",
		init,
		destroy,
		get_interface,
	#ifdef _WIN32
		win32config
	#else
		NULL
	#endif
	},
	init,
	close,
	pretag,
	verifymsg,
	loginfo,
	history,
	notify,
	precommit,
	postcommit,
	precommand,
	postcommand,
	premodule,
	postmodule,
	get_template,
	parse_keyword,
	prercsdiff,
	rcsdiff
};

static int init(const struct plugin_interface *plugin)
{
	return 0;
}

static int destroy(const struct plugin_interface *plugin)
{
	return 0;
}

static void *get_interface(const struct plugin_interface *plugin, unsigned interface_type, void *param)
{
	if(interface_type!=pitTrigger)
		return NULL;

	return (void*)&callbacks;
}

plugin_interface *get_plugin_interface()
{
	return &callbacks.plugin;
}

#ifdef _WIN32
BOOL CALLBACK ConfigDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char value[MAX_PATH];
	int nSel;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hWnd,IDC_COMBO1,CB_ADDSTRING,NULL,(LPARAM)L"MySQL");
		SendDlgItemMessage(hWnd,IDC_COMBO1,CB_ADDSTRING,NULL,(LPARAM)L"SQLite");
		SendDlgItemMessage(hWnd,IDC_COMBO1,CB_ADDSTRING,NULL,(LPARAM)L"PostgreSQL");
		SendDlgItemMessage(hWnd,IDC_COMBO1,CB_ADDSTRING,NULL,(LPARAM)L"Generic ODBC");
		SendDlgItemMessage(hWnd,IDC_COMBO1,CB_ADDSTRING,NULL,(LPARAM)L"SQL Server / MSDE");
		SendDlgItemMessage(hWnd,IDC_COMBO1,CB_ADDSTRING,NULL,(LPARAM)L"IBM DB2");
		nSel = 0;
		CGlobalSettings::GetGlobalValue("cvsnt","Plugins","AuditTrigger",nSel);
		if(!nSel)
		{
			EnableWindow(GetDlgItem(hWnd,IDC_COMBO1),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT1),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT2),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT5),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK1),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK2),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK3),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK4),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK5),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_BUTTON1),FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_BUTTON2),FALSE);

			nSel = 1;
			CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseType",nSel);
			SendDlgItemMessage(hWnd,IDC_COMBO1,CB_SETCURSEL,nSel,NULL);
		}
		else
		{
			SendDlgItemMessage(hWnd,IDC_CHECK6,BM_SETCHECK,1,NULL);

			nSel = 1;
			CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseType",nSel);
			SendDlgItemMessage(hWnd,IDC_COMBO1,CB_SETCURSEL,nSel,NULL);
			switch(nSel)
			{
			case 0: // MySql
			case 2: // Postgres
			case 3: // ODBC
			case 4: // SQL Server
			case 5: // DB2
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),TRUE);
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),TRUE);
				break;
			case 1: // SQLite
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),FALSE);
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),FALSE);
				break;
			}
		}
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseName",value,sizeof(value)))
			SetDlgItemText(hWnd,IDC_EDIT1,cvs::wide(value));
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabasePrefix",value,sizeof(value)))
			SetDlgItemText(hWnd,IDC_EDIT2,cvs::wide(value));
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseUsername",value,sizeof(value)))
			SetDlgItemText(hWnd,IDC_EDIT3,cvs::wide(value));
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabasePassword",value,sizeof(value)))
			SetDlgItemText(hWnd,IDC_EDIT4,cvs::wide(value));
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditDatabaseHost",value,sizeof(value)))
			SetDlgItemText(hWnd,IDC_EDIT5,cvs::wide(value));
		else
			SetDlgItemText(hWnd,IDC_EDIT5,L"localhost");
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogSessions",value,sizeof(value)))
			SendDlgItemMessage(hWnd,IDC_CHECK1,BM_SETCHECK,(WPARAM)atoi(value)?1:0,NULL);
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogCommits",value,sizeof(value)))
			SendDlgItemMessage(hWnd,IDC_CHECK2,BM_SETCHECK,(WPARAM)atoi(value)?1:0,NULL);
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogDiffs",value,sizeof(value)))
			SendDlgItemMessage(hWnd,IDC_CHECK3,BM_SETCHECK,(WPARAM)atoi(value)?1:0,NULL);
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogTags",value,sizeof(value)))
			SendDlgItemMessage(hWnd,IDC_CHECK4,BM_SETCHECK,(WPARAM)atoi(value)?1:0,NULL);
		if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","AuditLogHistory",value,sizeof(value)))
			SendDlgItemMessage(hWnd,IDC_CHECK5,BM_SETCHECK,(WPARAM)atoi(value)?1:0,NULL);
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CHECK6:
			nSel=SendDlgItemMessage(hWnd,IDC_CHECK6,BM_GETCHECK,NULL,NULL);
			EnableWindow(GetDlgItem(hWnd,IDC_COMBO1),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT1),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT2),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_EDIT5),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK1),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK2),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK3),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK4),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_CHECK5),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_BUTTON1),nSel?TRUE:FALSE);
			EnableWindow(GetDlgItem(hWnd,IDC_BUTTON2),nSel?TRUE:FALSE);
			if(nSel)
			{
				nSel = SendDlgItemMessage(hWnd,IDC_COMBO1,CB_GETCURSEL,NULL,NULL);
				switch(nSel)
				{
				case 0: // MySql
				case 2: // Postgres
				case 3: // ODBC
				case 4: // Mssql
				case 5: // DB2
					EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),TRUE);
					EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),TRUE);
					EnableWindow(GetDlgItem(hWnd,IDC_EDIT5),TRUE);
					break;
				case 1: // SQLite
					EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),FALSE);
					EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),FALSE);
					EnableWindow(GetDlgItem(hWnd,IDC_EDIT5),FALSE);
					break;
				}
			}
			return TRUE;
		case IDC_COMBO1:
			nSel = SendDlgItemMessage(hWnd,IDC_COMBO1,CB_GETCURSEL,NULL,NULL);
			switch(nSel)
			{
			case 0: // MySql
			case 2: // Postgres
			case 3: // ODBC
			case 4: // mssql
			case 5: // DB2
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),TRUE);
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),TRUE);
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT5),TRUE);
				break;
			case 1: // SQLite
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT3),FALSE);
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT4),FALSE);
				EnableWindow(GetDlgItem(hWnd,IDC_EDIT5),FALSE);
				break;
			}
			break;
		case IDC_BUTTON1: // Test connection
			{
				int nType;
				wchar_t name[256],prefix[256],user[256],password[256],host[256];
				cvs::string error;

				nType = SendDlgItemMessage(hWnd,IDC_COMBO1,CB_GETCURSEL,NULL,NULL);
				GetDlgItemText(hWnd,IDC_EDIT1,name,sizeof(name));
				GetDlgItemText(hWnd,IDC_EDIT2,prefix,sizeof(prefix));
				GetDlgItemText(hWnd,IDC_EDIT3,user,sizeof(user));
				GetDlgItemText(hWnd,IDC_EDIT4,password,sizeof(password));
				GetDlgItemText(hWnd,IDC_EDIT5,host,sizeof(host));

				HCURSOR hCursor = SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_WAIT)));
				CSqlConnection *pDb = Connect(nType,cvs::narrow(name),cvs::narrow(host),cvs::narrow(user),cvs::narrow(password),error);
				SetCursor(hCursor);
				if(!pDb)
				{
					cvs::string tmp;
					cvs::sprintf(tmp,80,"Connection failed:\n\n%s",error.c_str());
					MessageBox(hWnd,cvs::wide(tmp.c_str()),L"Connection test",MB_ICONSTOP);
				}
				else
				{
					pDb->Close();
					delete pDb;
					MessageBox(hWnd,L"Connection OK",L"Connection test",MB_ICONINFORMATION);
				}
			}
			break;
		case IDC_BUTTON2: // Create tables
			{
				cvs::string fn;

				int nType;
				wchar_t name[256],prefix[256],user[256],password[256],host[256];
				cvs::string error;

				fn = CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDLib);
				fn+="/sql/create_tables_";

				nSel = SendDlgItemMessage(hWnd,IDC_COMBO1,CB_GETCURSEL,NULL,NULL);
				switch(nSel)
				{
				case 0: // MySql
					fn+="mysql"; break;
				case 1: // SQLite
					fn+="sqlite"; break;
				case 2: // Postgres
					fn+="pgsql"; break;
				case 3: // ODBC
					fn+="mssql"; break; // FIXME:  No generic ODBC syntax
				case 4: // mssql
					fn+="mssql"; break;
				case 5: // db2
					fn+="db2"; break; 
				default:
					fn+="xxxx"; break;
				}

				fn+=".sql";
				if(!CFileAccess::exists(fn.c_str()))
				{
					MessageBox(hWnd,L"Script not found",L"Create tables",MB_ICONSTOP);
					return TRUE;
				}
				
				nType = SendDlgItemMessage(hWnd,IDC_COMBO1,CB_GETCURSEL,NULL,NULL);
				GetDlgItemText(hWnd,IDC_EDIT1,name,sizeof(name));
				GetDlgItemText(hWnd,IDC_EDIT2,prefix,sizeof(prefix));
				GetDlgItemText(hWnd,IDC_EDIT3,user,sizeof(user));
				GetDlgItemText(hWnd,IDC_EDIT4,password,sizeof(password));
				GetDlgItemText(hWnd,IDC_EDIT5,host,sizeof(host));

				HCURSOR hCursor = SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_WAIT)));
				CSqlConnection *pDb = Connect(nType,cvs::narrow(name),cvs::narrow(host),cvs::narrow(user),cvs::narrow(password),error);
				SetCursor(hCursor);
				if(!pDb)
				{
					cvs::string tmp;
					cvs::sprintf(tmp,80,"Connection failed:\n\n%s",error.c_str());
					MessageBox(hWnd,cvs::wide(tmp.c_str()),L"Create tables",MB_ICONSTOP);
				}
				else
				{
					CFileAccess acc;
					cvs::string line,comp_line;
					if(!acc.open(fn.c_str(),"r"))
					{
						pDb->Close();
						delete pDb;
						MessageBox(hWnd,L"Script could not be opened",L"Create tables",MB_ICONSTOP);
						return TRUE;
					}

					comp_line="";
					while(acc.getline(line))
					{
						if(line.size()<2 || !strncmp(line.c_str(),"--",2))
							continue;
						comp_line+=" "+line;
						if(line[line.size()-1]!=';')
							continue;
						size_t pos;
						while((pos=comp_line.find("%PREFIX%"))!=cvs::string::npos)
							comp_line.replace(pos,8,cvs::narrow(prefix));
						pDb->Execute("%s",comp_line.c_str());
						if(pDb->Error())
						{
							MessageBox(hWnd,cvs::wide(pDb->ErrorString()),L"Create tables",MB_ICONSTOP|MB_OK);
							pDb->Close();
							delete pDb;
							return TRUE;
						}
						comp_line="";
					}
					pDb->Close();
					delete pDb;
					MessageBox(hWnd,L"OK",L"Create tables",MB_ICONINFORMATION);
				}
			}
			break;
		case IDOK:
			{
				wchar_t wvalue[256];

				nSel=SendDlgItemMessage(hWnd,IDC_CHECK6,BM_GETCHECK,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","Plugins","AuditTrigger",nSel);
				nSel = SendDlgItemMessage(hWnd,IDC_COMBO1,CB_GETCURSEL,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditDatabaseType",nSel);
				GetDlgItemText(hWnd,IDC_EDIT1,wvalue,sizeof(wvalue)/sizeof(wvalue[0]));
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditDatabaseName",cvs::narrow(wvalue));
				GetDlgItemText(hWnd,IDC_EDIT2,wvalue,sizeof(wvalue)/sizeof(wvalue[0]));
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditDatabasePrefix",cvs::narrow(wvalue));
				GetDlgItemText(hWnd,IDC_EDIT3,wvalue,sizeof(wvalue)/sizeof(wvalue[0]));
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditDatabaseUsername",cvs::narrow(wvalue));
				GetDlgItemText(hWnd,IDC_EDIT4,wvalue,sizeof(wvalue)/sizeof(wvalue[0]));
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditDatabasePassword",cvs::narrow(wvalue));
				GetDlgItemText(hWnd,IDC_EDIT5,wvalue,sizeof(wvalue)/sizeof(wvalue[0]));
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditDatabaseHost",cvs::narrow(wvalue));
				nSel=SendDlgItemMessage(hWnd,IDC_CHECK1,BM_GETCHECK,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditLogSessions",nSel);
				nSel=SendDlgItemMessage(hWnd,IDC_CHECK2,BM_GETCHECK,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditLogCommits",nSel);
				nSel=SendDlgItemMessage(hWnd,IDC_CHECK3,BM_GETCHECK,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditLogDiffs",nSel);
				nSel=SendDlgItemMessage(hWnd,IDC_CHECK4,BM_GETCHECK,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditLogTags",nSel);
				nSel=SendDlgItemMessage(hWnd,IDC_CHECK5,BM_GETCHECK,NULL,NULL);
				CGlobalSettings::SetGlobalValue("cvsnt","PServer","AuditLogHistory",nSel);
			}
		case IDCANCEL:
			EndDialog(hWnd,LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

int win32config(const struct plugin_interface *ui, void *wnd)
{
	HWND hWnd = (HWND)wnd;
	int ret = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, ConfigDlgProc);
	return ret==IDOK?0:-1;
}
#endif

