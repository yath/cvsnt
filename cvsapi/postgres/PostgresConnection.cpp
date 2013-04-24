/*
	CVSNT Generic API
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
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#endif
#include <config.h>
#include "../lib/api_system.h"

#ifdef HAVE_LIBPQ_FE_H
#include <libpq-fe.h>
#else
// else what??
#endif

#include "../cvs_string.h"
#include "../FileAccess.h"
#include "../ServerIO.h"
#include "PostgresConnection.h"
#include "PostgresRecordset.h"

extern "C" CSqlConnection *Postgres_Alloc()
{
        return new CPostgresConnection;
}

CPostgresConnection::CPostgresConnection()
{
	m_pDb=NULL;
	m_lasterr=PGRES_COMMAND_OK;
}

CPostgresConnection::~CPostgresConnection()
{
	Close();
}

bool CPostgresConnection::Create(const char *host, const char *database, const char *username, const char *password)
{
	if(!Open(host,"template1",username,password))
		return false;
	Execute("create database %s",database);
	if(Error())
		return false;
	Close();
	return Open(host,database,username,password);
}

bool CPostgresConnection::Open(const char *host, const char *database, const char *username, const char *password)
{
	return __Open(host,database,username,password);
}

bool CPostgresConnection::__Open(const char *host, const char *database, const char *username, const char *password)
{
#ifdef _WIN32
	__try
	{
#endif
		char tmp[1024];
		snprintf(tmp,sizeof(tmp),"host = '%s' dbname = '%s' user = '%s' password = '%s'",host,database,username,password);

		m_pDb = PQconnectdb(tmp);
		if(!m_pDb)
			return false;

		if(PQstatus(m_pDb)==CONNECTION_BAD)
			return false;

		PQsetClientEncoding(m_pDb,"UNICODE");
#ifdef _WIN32
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
#endif
	return true;
}

bool CPostgresConnection::Close()
{
	if(m_pDb)
		PQfinish(m_pDb);

	m_pDb=NULL;
	return true;
}

bool CPostgresConnection::IsOpen()
{
	if(m_pDb && PQstatus(m_pDb)!=CONNECTION_BAD)
		return true;
	return false;
}

CSqlRecordsetPtr CPostgresConnection::Execute(const char *string, ...)
{
	cvs::string str;
	va_list va;
	va_start(va,string);
	cvs::vsprintf(str,64,string,va);
	va_end(va);

	CPostgresRecordset *rs = new CPostgresRecordset();

	/* postgres doesn't support the standard questionmark operator, instead using '$n'.
	   preparse the string looking for these and replacing them.  This routine is probably
	   not very robust. */
	size_t pos = 0;
	int var = 1;
	char varstr[32];
	bool quote = false;
	while(pos<str.size())
	{
		char c=str[pos];
		if(!quote && c=='\'')
			quote=true;
		else if(quote && c=='\'')
			quote=false;
		else if(!quote && c=='?')
		{
			snprintf(varstr,sizeof(varstr),"$%d",var++);
			str.replace(pos,1,varstr);
		}
		pos++;
	}

	CServerIo::trace(3,"%s",str.c_str());

	PGresult *rslt;
	int numParams = (int)m_bindVars.size();
	Oid *paramTypes = NULL;
	char **paramValues = NULL;
	int *paramLength = NULL;
	int *paramFormats = NULL;
	// Appears to be impossible to define in VC.. Probably an MS bug.
	typedef char __dummy[16];
	__dummy *paramVars = NULL;
	if(numParams)
	{
		paramTypes = new Oid[numParams];
		paramValues = new char *[numParams];
		paramLength = new int[numParams];
		paramFormats = new int[numParams];
		paramVars = new char[numParams][16];

		int n = 0;
		for(std::map<int,CSqlVariant>::iterator i = m_bindVars.begin(); i!=m_bindVars.end(); ++i)
		{
			paramFormats[n]=1;
			switch(i->second.type())
			{
			case CSqlVariant::vtNull:
				paramTypes[n]=0; 
				paramValues[n]=0;
				paramLength[n]=0;
				break;
			case CSqlVariant::vtChar:
			case CSqlVariant::vtUChar:
				paramTypes[n]=18;
				*(char*)paramVars[n]=(char)i->second;
				paramValues[n]=paramVars[n];
				paramLength[n]=sizeof(char);
				break;
			case CSqlVariant::vtShort:
			case CSqlVariant::vtUShort:
				paramTypes[n]=21;
				*(short*)paramVars[n]=(short)i->second;
				paramValues[n]=paramVars[n];
				paramLength[n]=sizeof(short);
				break;
			case CSqlVariant::vtInt:
			case CSqlVariant::vtUInt:
			case CSqlVariant::vtLong:
			case CSqlVariant::vtULong:
				if(sizeof(int)==4)
				{
					paramTypes[n]=23; 
					*(int*)paramVars[n]=(int)i->second;
					paramValues[n]=paramVars[n];
					paramLength[n]=sizeof(int);
					break;
				}
			/* fall through (64bit systems) */
			case CSqlVariant::vtLongLong:
			case CSqlVariant::vtULongLong:
				paramTypes[n]=20;
	#ifdef _WIN32
				*(__int64*)paramVars[n]=(__int64)i->second;
				paramValues[n]=paramVars[n];
				paramLength[n]=sizeof(__int64);
	#else
				*(long long*)paramVars[n]=(long long)i->second;
				paramValues[n]=paramVars[n];
				paramLength[n]=sizeof(long long);
	#endif
				break;
			case CSqlVariant::vtString:
			case CSqlVariant::vtWString:
				{
				paramTypes[n]=25;
				const char *s = i->second;
				paramLength[n]=(int)strlen(s);
				paramValues[n]=(char*)s;
				}
				break;
			}
			n++;
		}
	}


	rslt = PQexecParams(m_pDb, str.c_str(), numParams, paramTypes, paramValues, paramLength, paramFormats, 1);

	if(paramTypes)
		delete[] paramTypes;
	if(paramValues)
		delete[] paramValues;
	if(paramLength)
		delete[] paramLength;
	if(paramFormats)
		delete[] paramFormats;
	if(paramVars)
		delete[] paramVars;

	if(!rslt)
	{
		m_lasterr = PGRES_FATAL_ERROR;
		return rs;
	}

	m_lasterr = PQresultStatus(rslt);
	if(m_lasterr == PGRES_BAD_RESPONSE || m_lasterr == PGRES_NONFATAL_ERROR || m_lasterr == PGRES_FATAL_ERROR)
	{
		if(rslt)
			m_lasterrmsg = PQresultErrorMessage(rslt);
		else
			m_lasterrmsg = "";
		if(m_lasterrmsg.size() && m_lasterrmsg[m_lasterrmsg.size()-1]=='\n')
			m_lasterrmsg.resize(m_lasterrmsg.size()-1);
		PQclear(rslt);
		return rs;
	}

	rs->Init(m_pDb,rslt);

	m_bindVars.clear();

	return rs;
}

bool CPostgresConnection::Error() const
{
	if(!m_pDb) return true;
	if(PQstatus(m_pDb)==CONNECTION_BAD) return true;
	if(m_lasterr == PGRES_BAD_RESPONSE || m_lasterr == PGRES_NONFATAL_ERROR || m_lasterr == PGRES_FATAL_ERROR)
		return true;
	return false;
}

const char *CPostgresConnection::ErrorString() 
{
	if(!m_pDb)
		return "Database not created or couldn't find libpq.dll";
	if(PQstatus(m_pDb)!=CONNECTION_OK)
		return PQerrorMessage(m_pDb);
	if(m_lasterrmsg.size())
		return m_lasterrmsg.c_str();
	return PQresStatus((ExecStatusType)m_lasterr);
}

unsigned CPostgresConnection::GetInsertIdentity(const char *table_hint) 
{
	cvs::string str;
	cvs::sprintf(str,80,"select currval('%s_id_seq')",table_hint);
	PGresult *rslt = PQexec(m_pDb, str.c_str());
	if(!PQntuples(rslt) || !PQnfields(rslt))
	{
		CServerIo::trace(1,"Postgres GetInsertIdentity(%s) failed",table_hint);
		return 0;
	}

	unsigned long ident;
	if(sscanf(PQgetvalue(rslt,0,0),"%lu",&ident)!=1)
	{
		CServerIo::trace(1,"Postgres GetInsertIdentity(%s) failed (bogus value)",table_hint);
		return 0;
	}

	PQclear(rslt);

	return ident;
}

bool CPostgresConnection::BeginTrans()
{
	PGresult *rslt = PQexec(m_pDb, "BEGIN TRANSACTION");
	m_lasterr = PQresultStatus(rslt);
	PQclear(rslt);
	if(m_lasterr == PGRES_BAD_RESPONSE || m_lasterr == PGRES_NONFATAL_ERROR || m_lasterr == PGRES_FATAL_ERROR)
		return false;
	return true;
}

bool CPostgresConnection::CommitTrans()
{
	PGresult *rslt = PQexec(m_pDb, "COMMIT TRANSACTION");
	m_lasterr = PQresultStatus(rslt);
	PQclear(rslt);
	if(m_lasterr == PGRES_BAD_RESPONSE || m_lasterr == PGRES_NONFATAL_ERROR || m_lasterr == PGRES_FATAL_ERROR)
		return false;
	return true;
}

bool CPostgresConnection::RollbackTrans()
{
	PGresult *rslt = PQexec(m_pDb, "ROLLBACK TRANSACTION");
	m_lasterr = PQresultStatus(rslt);
	PQclear(rslt);
	if(m_lasterr == PGRES_BAD_RESPONSE || m_lasterr == PGRES_NONFATAL_ERROR || m_lasterr == PGRES_FATAL_ERROR)
		return false;
	return true;
}

bool CPostgresConnection::Bind(int variable, CSqlVariant value)
{
	m_bindVars[variable]=value;
	return true;
}
