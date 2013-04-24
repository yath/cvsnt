/*
	CVSNT Generic API
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
#ifndef RPCBASE__H
#define RPCBASE__H

class CrpcBase
{
public:
	CVSAPI_EXPORT CrpcBase();
	CVSAPI_EXPORT virtual ~CrpcBase();

	struct rpcObject
	{
		virtual bool Marshall(CXmlNode *params) =0;
	};

	CVSAPI_EXPORT static CXmlNode *createNewParams(CXmlTree& tree);

	CVSAPI_EXPORT static bool addParam(CXmlNode *params, const char *name, const char *value);
	CVSAPI_EXPORT static bool addParam(CXmlNode *params, const char *name, int value);
	CVSAPI_EXPORT static bool addParam(CXmlNode *params, const char *name, rpcObject *obj);

	template<typename _Ty>
		static bool addParamVector(CXmlNode *params, std::vector<_Ty>& ar)
		{
			CXmlNode *array = params->NewNode("array",NULL);
			CXmlNode *data = array->NewNode("data", NULL);
			for(size_t n=0; n<ar.size(); n++)
				if(!addParam(data,NULL,&ar[n]))
					return false;
			return true;
		}

	CVSAPI_EXPORT static bool rpcInt(CXmlNode *param, const char *name, int& value);
	CVSAPI_EXPORT static bool rpcString(CXmlNode *param, const char *name, cvs::string& value);
	CVSAPI_EXPORT static bool rpcArray(CXmlNode* param, const char *name, CXmlNode*& node);
	CVSAPI_EXPORT static bool rpcObj(CXmlNode* param, const char *name, rpcObject *obj);

	CVSAPI_EXPORT static CXmlNode *rpcFault(CXmlTree& tree, int err, const char *error);
	CVSAPI_EXPORT static CXmlNode *rpcResponse(CXmlNode* result);
	CVSAPI_EXPORT static CXmlNode *rpcCall(const char *method, CXmlNode *param);
};

#endif
