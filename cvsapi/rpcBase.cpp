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
#include "lib/api_system.h"
#include "cvs_string.h"
#include "cvs_smartptr.h"
#include "Codepage.h"
#include "Xmlapi.h"
#include "rpcBase.h"

CrpcBase::CrpcBase()
{
}

CrpcBase::~CrpcBase()
{
}

CXmlNode *CrpcBase::createNewParams(CXmlTree& tree)
{
	return new CXmlNode(&tree, CXmlNode::XmlTypeNode, "params", NULL);
}

bool CrpcBase::addParam(CXmlNode *params, const char *name, const char *value)
{
	CXmlNode *param;
	
	if(!strcmp(params->GetName(),"params"))
		param = params->NewNode("param", NULL);
	else if(!strcmp(params->GetName(),"struct"))
	{
		param = params->NewNode("member", NULL);
		if(name)
			param->NewNode("name",name);
	}
	else
		param = params; // array (name will also be null)
	CXmlNode *val = param->NewNode("value",NULL);
	val->NewNode("string",value);
	return true;
}

bool CrpcBase::addParam(CXmlNode *params, const char *name, int value)
{
	char tmp[32];
	snprintf(tmp,sizeof(tmp),"%d",value);
	CXmlNode *param;
	
	if(!strcmp(params->GetName(),"params"))
		param = params->NewNode("param", NULL);
	else if(!strcmp(params->GetName(),"struct"))
	{
		param = params->NewNode("member", NULL);
		if(name)
			param->NewNode("name",name);
	}
	else
		param = params; // array (name will also be null)
	CXmlNode *val = param->NewNode("value",NULL);
	val->NewNode("i4",tmp);
	return true;
}

bool CrpcBase::addParam(CXmlNode *params, const char *name, rpcObject *obj)
{
	CXmlNode *param;
	
	if(!strcmp(params->GetName(),"params"))
		param = params->NewNode("param", NULL);
	else if(!strcmp(params->GetName(),"struct"))
	{
		param = params->NewNode("member", NULL);
		if(name)
			param->NewNode("name",name);
	}
	else
		param = params; // array (name will also be null)
	CXmlNode *val = param->NewNode("value",NULL);
	return obj->Marshall(val);
}

bool CrpcBase::rpcInt(CXmlNode *param, const char *name, int& value)
{
	cvs::string fnd;
	CXmlNode *val, *obj;

	if(!strcmp(param->GetName(),"param")) 
		val=param->getChild(0);
	else
		val = param;
	if(val && !strcmp(val->GetName(),"struct"))
	{
		if(name)
		{
			cvs::sprintf(fnd,64,"member[@name='%s']",name);
			val = val->Lookup(fnd.c_str());
			if(!val)
				return false;
		}
		else
			val = param->getChild(0);
		val = val->Lookup("value");
	}
	if(!val || strcmp(val->GetName(),"value"))
		return false;
	obj = val->getChild(0);
	if(!obj || strcmp(obj->GetName(),"i4"))
		return false;
	value = atoi(obj->GetValue());
	return true;
}

bool CrpcBase::rpcString(CXmlNode *param, const char *name, cvs::string& value)
{
	cvs::string fnd;
	CXmlNode *val, *obj;

	if(!strcmp(param->GetName(),"param")) 
		val=param->getChild(0);
	else
		val = param;
	if(val && !strcmp(val->GetName(),"struct"))
	{
		if(name)
		{
			cvs::sprintf(fnd,64,"member[@name='%s']",name);
			val = val->Lookup(fnd.c_str());
			if(!val)
				return false;
		}
		else
			val = param->getChild(0);
		val = val->Lookup("value");
	}
	if(!val || strcmp(val->GetName(),"value"))
		return false;
	obj = val->getChild(0);
	if(!obj || strcmp(obj->GetName(),"string"))
		return false;
	value = obj->GetValue();
	return true;
}

bool CrpcBase::rpcArray(CXmlNode* param, const char *name, CXmlNode*& node)
{
	CXmlNode *val;
	if(!strcmp(param->GetName(),"param"))
		val=param->getChild(0);
	else
		val = param;
	if(!val || strcmp(val->GetName(),"array"))
		return false;

	if(!node)
	{
		CXmlNode *data = val->getChild(0);
		if(!data || strcmp(data->GetName(),"data"))
			return false;
		node = data->getChild(0);
		return true;
	}
	else
	{
		node = node->getParent()->Next();
		if(!node || strcmp(node->GetName(),"data"))
			return false;
		node = node->getChild(0);
		return true;
	}
}

bool CrpcBase::rpcObj(CXmlNode* param, const char *name, rpcObject *rpcObj)
{
	cvs::string fnd;
	CXmlNode *val, *obj;

	if(!strcmp(param->GetName(),"param")) 
		val=param->getChild(0);
	else
		val = param;
	if(val && !strcmp(val->GetName(),"struct"))
	{
		if(name)
		{
			cvs::sprintf(fnd,64,"member[@name='%s']",name);
			val = val->Lookup(fnd.c_str());
			if(!val)
				return false;
		}
		else
			val = param->getChild(0);
		val = val->Lookup("value");
	}
	if(!val || strcmp(val->GetName(),"value"))
		return false;
	obj = val->getChild(0);
	if(!obj || strcmp(obj->GetName(),"struct"))
		return false;
	return rpcObj->Marshall(obj);
}

CXmlNode *CrpcBase::rpcFault(CXmlTree& tree, int err, const char *error)
{
	CXmlNode *fault = new CXmlNode(&tree, CXmlNode::XmlTypeNode, "fault", NULL);
	CXmlNode *value = fault->NewNode("value", NULL);
	CXmlNode *struc = value->NewNode("struct", NULL);
	addParam(struc,"faultCode",err);
	addParam(struc,"faultString",error);
	return fault;
}

CXmlNode *CrpcBase::rpcResponse(CXmlNode* result)
{
	CXmlNode *response = new CXmlNode(result->getTree(), CXmlNode::XmlTypeNode, "methodResponse", NULL);
	CXmlNode *params = response->NewNode("params",NULL);
	params->Paste(result);
	return response;
}

CXmlNode *CrpcBase::rpcCall(const char *method, CXmlNode *param)
{
	CXmlNode *methodNode = new CXmlNode(param->getTree(), CXmlNode::XmlTypeNode, "methodCall", NULL);
	methodNode->NewNode("methodName",method);
	CXmlNode *params = methodNode->NewNode("params",NULL);
	params->Paste(param);
	return methodNode;
}
