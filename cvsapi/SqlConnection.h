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
#ifndef SQLCONNECTION__H
#define SQLCONNECTION__H

#include "SqlVariant.h"
#include "SqlRecordset.h"

enum SqlConnectionType
{
  sqtSqlite,
  sqtMysql,
  sqtPostgres,
  sqtOdbc,
  sqtMssql,
  sqtFirebird,
  sqtDb2
};

class CSqlConnection
{
public:
	CSqlConnection() { }
	virtual ~CSqlConnection() { }

	static CVSAPI_EXPORT CSqlConnection* Alloc(SqlConnectionType type, const char *library_dir);

	virtual bool Create(const char *hostname, const char *database, const char *username, const char *password) =0;
	virtual bool Open(const char *hostname, const char *database, const char *username, const char *password) =0;
	virtual bool IsOpen() =0;
	virtual bool Close() =0;
	virtual bool Bind(int variable, CSqlVariant value) =0;
	virtual CSqlRecordsetPtr Execute(const char *string, ...) =0;
	virtual bool Error() const =0;
	virtual const char *ErrorString() =0;
	virtual unsigned GetInsertIdentity(const char *table_hint) =0;
	virtual bool BeginTrans() = 0;
	virtual bool CommitTrans() = 0;
	virtual bool RollbackTrans() = 0;
};

#endif
