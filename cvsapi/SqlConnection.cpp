/*
	CVSNT Generic API
    Copyright (C) 2004 Tony Hoyle and March-Hare Software Ltd

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
#include <config.h>
#include "lib/api_system.h"
#include "cvs_string.h"
#include "ServerIO.h"
#include "SqlConnection.h"
#include "LibraryAccess.h"

#ifdef _WIN32
#define STATIC_DB /* We can use late binding in Win32 to be more efficient */
#else
#undef STATIC_DB
#endif

extern "C" CSqlConnection *SQLite_Alloc();
extern "C" CSqlConnection *MySql_Alloc();
extern "C" CSqlConnection *Postgres_Alloc();
extern "C" CSqlConnection *Odbc_Alloc();
extern "C" CSqlConnection *Mssql_Alloc();
extern "C" CSqlConnection *Firebird_Alloc();
extern "C" CSqlConnection *Db2_Alloc();

CSqlConnection* CSqlConnection::Alloc(SqlConnectionType type, const char *dir)
{
#ifdef STATIC_DB
	switch(type)
	{
#ifdef HAVE_SQLITE
	case sqtSqlite:
		CServerIo::trace(3,"Connecting to SQLite");
		return SQLite_Alloc();
#endif
#ifdef HAVE_MYSQL
	case sqtMysql:
		CServerIo::trace(3,"Connecting to MySql");
		return MySql_Alloc();
#endif
#ifdef HAVE_POSTGRES
	case sqtPostgres:
		CServerIo::trace(3,"Connecting to Postgres");
		return Postgres_Alloc();
#endif
#ifdef HAVE_ODBC
	case sqtOdbc:
		CServerIo::trace(3,"Connecting to Odbc");
		return Odbc_Alloc();
#endif
#ifdef HAVE_MSSQL
	case sqtMssql:
		CServerIo::trace(3,"Connecting to MS-Sql");
		return Mssql_Alloc();
#endif
#ifdef HAVE_FIREBIRD
	case sqtFirebird:
		CServerIo::trace(3,"Connecting to Firebird");
		return Firebird_Alloc();
#endif
#ifdef HAVE_DB2
	case sqtDb2:
		CServerIo::trace(3,"Connecting to DB2");
		return Db2_Alloc();
#endif
	default:
		return NULL;
	}
#else /* not STATIC_DB */
	CLibraryAccess la;
	CSqlConnection* (*pNewSqlConnection)() = NULL;

	switch(type)
	{
	case sqtSqlite:
		CServerIo::trace(3,"Connecting to SQLite");
		if(!la.Load("sqlite"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewSqlConnection = (CSqlConnection*(*)())la.GetProc("SQLite_Alloc");
		break;
	case sqtMysql:
		CServerIo::trace(3,"Connecting to MySql");
		if(!la.Load("mysql"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewSqlConnection = (CSqlConnection*(*)())la.GetProc("MySql_Alloc");
		break;
	case sqtPostgres:
		CServerIo::trace(3,"Connecting to Postgres");
		if(!la.Load("postgres"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewSqlConnection = (CSqlConnection*(*)())la.GetProc("Postgres_Alloc");
		break;
	case sqtOdbc:
		CServerIo::trace(3,"Connecting to Odbc");
		if(!la.Load("odbc"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewSqlConnection = (CSqlConnection*(*)())la.GetProc("Odbc_Alloc");
		break;
	case sqtMssql:
		return NULL; // Win32 only (which is normally static)
	case sqtFirebird:
		CServerIo::trace(3,"Connecting to Firebird");
		if(!la.Load("firebird"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewSqlConnection = (CSqlConnection*(*)())la.GetProc("Firebird_Alloc");
		break;
	case sqtDb2:
		CServerIo::trace(3,"Connecting to DB2");
		if(!la.Load("db2"SHARED_LIBRARY_EXTENSION,dir))
			return false;
		pNewSqlConnection = (CSqlConnection*(*)())la.GetProc("Db2_Alloc");
		break;
	default:
		return NULL;
	}
	if(!pNewSqlConnection)
		return NULL;
	CSqlConnection *conn = pNewSqlConnection();
	la.Detach(); // Add a reference so the library never unloads
	return conn;
#endif
}

