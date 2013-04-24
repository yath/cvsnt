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
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#endif
#include <config.h>
#include "../lib/api_system.h"

#ifndef _WIN32
#include <mysql.h>
#else
/* Win32 includes its own copy */
#include "mysql-3.23/mysql.h"
#endif
#include <string>
#include "../ServerIO.h"
#include "../cvs_string.h"
#include "MySqlRecordset.h"
#include "MySqlConnection.h"
#include "MySqlRecordset.h"

extern "C" CSqlConnection *MySql_Alloc()
{
	return new CMySqlConnection;
}

CMySqlConnection::CMySqlConnection()
{
	m_connect = NULL;
}

CMySqlConnection::~CMySqlConnection()
{
	Close();
}

bool CMySqlConnection::Create(const char *host, const char *database, const char *username, const char *password)
{
	if(!Open(host,"mysql",username,password))
		return false;
	Execute("create database %s",database);
	if(Error())
		return false;
	Close();
	return Open(host,database,username,password);
}

bool CMySqlConnection::Open(const char *host, const char *database, const char *username, const char *password)
{
#ifdef _WIN32
	__try
	{
#endif
		MYSQL *mysql =  mysql_init(NULL);
		if(!mysql)
			return false;

		m_connect = mysql_real_connect(mysql,host,username,password,database,0,NULL,0);
		if(!m_connect)
		{
			m_connect = mysql;
			return false;
		}

#ifdef _WIN32
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
#endif
	return true;
}

bool CMySqlConnection::Close()
{
	if(m_connect)
		mysql_close(m_connect);
	m_connect = NULL;
	return true;
}

bool CMySqlConnection::IsOpen()
{
	if(!m_connect)
		return false;
	if(!m_connect->host)
		return false;
	return true;
}

CSqlRecordsetPtr CMySqlConnection::Execute(const char *string, ...)
{
	cvs::string str;
	va_list va;
	va_start(va,string);
	cvs::vsprintf(str,64,string,va);
	va_end(va);

	CMySqlRecordset *rs = new CMySqlRecordset;

	/* Unfortunately the mysql interfaces changed with each release... this makes
	   it a complete bitch to write a wrapper for.  This is based on mysql-3.23,
	   to avoid licensing issues */
	/* Mysql support is going away due to the licensing, so bugs here won't have a high
	   priority */


	/* Mysql doesn't support prepared statements, so we have to modify the string.. */
	size_t pos = 0;
	int num = 0;

	while((pos=str.find('\\',pos))!=cvs::string::npos)
	{
		str.insert(pos,"\\");
		pos+=2;
	}

	pos=0;
	while((pos=str.find('?',pos))!=cvs::string::npos && num<(int)m_bindVars.size())
	{
		cvs::string esc;
		bool number = m_bindVars[num].isNumeric();
		const char *v = m_bindVars[num];
		size_t l = strlen(v);
		esc.resize(l*2+1);
		esc.resize(mysql_real_escape_string(m_connect,(char*)esc.data(),v,(unsigned long)l));
		if(!number)
		{
			esc.insert(0,"\'");
			esc+='\'';
		}
		str.replace(pos,1,esc);
		pos+=esc.length();
	}

	CServerIo::trace(3,"%s",str.c_str());

	if(mysql_real_query(m_connect,str.c_str(),(unsigned long)str.length()))
		return rs;
	if((rs->m_result = mysql_use_result(m_connect))==NULL)
		return rs;
	
	if(!rs->Init())
		return rs;

	m_bindVars.clear();

	return rs;
}

bool CMySqlConnection::Error() const
{
	if(!m_connect)
		return true;
	if(mysql_errno(m_connect))
		return true;
	return false;
}

const char *CMySqlConnection::ErrorString() 
{
	if(!m_connect)
		return "Open failed";
	if(!Error())
		return "No error";
	const char *p=mysql_error(m_connect);
	return p;
}

unsigned CMySqlConnection::GetInsertIdentity(const char *table_hint) 
{
	return (unsigned)mysql_insert_id(m_connect);
}

bool CMySqlConnection::BeginTrans()
{
	mysql_query(m_connect, "START TRANSACTION");
	return true;
}

bool CMySqlConnection::CommitTrans()
{
	mysql_query(m_connect, "COMMIT");
	return true;
}

bool CMySqlConnection::RollbackTrans()
{
	mysql_query(m_connect, "ROLLBACK");
	return true;
}

bool CMySqlConnection::Bind(int variable, CSqlVariant value)
{
	m_bindVars[variable]=value;
	return true;
}
