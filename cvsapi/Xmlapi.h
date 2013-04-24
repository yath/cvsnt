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
#ifndef XMLAPI__H
#define XMLAPI__H

#include <map>
#include <vector>
#include <string>

#ifdef INC_EXPAT
#include <expat.h>
#endif

class CXmlTree;
class CXmlNode;

typedef cvs::smartptr<CXmlNode> CXmlNodePtr;

class CXmlNode
{
	friend class CXmlTree;
public:
	typedef std::vector<CXmlNodePtr> ChildArray_t;

	enum XmlTypeEnum
	{
		XmlTypeNode,
		XmlTypeAttribute
	};

	CVSAPI_EXPORT CXmlNode(CXmlTree* tree) { m_tree = tree; XmlType=XmlTypeNode; Parent=NULL; StartLineNumber=EndLineNumber=0; keyNum=0; sorted=false; }
	CVSAPI_EXPORT CXmlNode(CXmlTree* tree, XmlTypeEnum Type, const char *name, const char *value) { m_tree = tree; XmlType=Type; Parent=NULL; this->name=name; if(value) text=text; StartLineNumber=EndLineNumber=0; keyNum=0; sorted=false; }
	CVSAPI_EXPORT CXmlNode(const CXmlNode& other);
	CVSAPI_EXPORT virtual ~CXmlNode() { }
	CVSAPI_EXPORT CXmlNode *NewAttribute(const char *name, const char *value) { return _New(XmlTypeAttribute, name, value); }
	CVSAPI_EXPORT CXmlNode *NewNode(const char *name, const char *value) { return _New(XmlTypeNode, name, value); }
	CVSAPI_EXPORT void SetValue(const char *value) { text = value?value:""; }
	CVSAPI_EXPORT void AppendValue(const char *value, size_t len) { if(value) text.append(value,len); }
	CVSAPI_EXPORT const char *GetValue() const { return text.c_str(); }
	CVSAPI_EXPORT const char *GetName() const { return name.c_str(); }
	CVSAPI_EXPORT XmlTypeEnum getType() const { return XmlType; }
	CVSAPI_EXPORT CXmlNode *Lookup(const char *path, bool autoCreate = false);
	CVSAPI_EXPORT CXmlNode *Next();
	CVSAPI_EXPORT CXmlNode *Previous();
	CVSAPI_EXPORT CXmlNode *getParent() const { return this->Parent; }
	CVSAPI_EXPORT bool Delete(CXmlNode *child);
	CVSAPI_EXPORT bool BatchDelete();
	CVSAPI_EXPORT bool Prune();
	CVSAPI_EXPORT CXmlNode *Copy();
	CVSAPI_EXPORT bool Paste(CXmlNode *from);
	CVSAPI_EXPORT size_t size() const { return Children.size(); }
	CVSAPI_EXPORT ChildArray_t::iterator begin() { return Children.begin(); }
	CVSAPI_EXPORT ChildArray_t::iterator end() { return Children.end(); }
	CVSAPI_EXPORT int getStartLine() const { return StartLineNumber; }
	CVSAPI_EXPORT int getEndLine() const { return EndLineNumber; }
	CVSAPI_EXPORT CXmlNode *getChild(size_t child) const { if(child>Children.size()) return NULL; return Children[child]; }
	CVSAPI_EXPORT CXmlTree* getTree() const { return m_tree; }

	CVSAPI_EXPORT bool WriteXmlFile(FILE *file) const;
	CVSAPI_EXPORT bool WriteXmlToString(cvs::string& string) const;
	CVSAPI_EXPORT bool Sort() { return SortMe(); }

protected:
	std::string name;
	std::string text;
	int keyNum;
	bool sorted;
	ChildArray_t Children;
	CXmlNode *Parent;
	XmlTypeEnum XmlType;
	int StartLineNumber,EndLineNumber;
	CXmlTree *m_tree;

	CXmlNode *_New(XmlTypeEnum Type, const char *name, const char *value);
	bool WriteXmlNode(FILE *file, int indent) const;
	bool WriteXmlNodeToString(cvs::string& string, int indent) const;
	ChildArray_t::iterator FindIterator(CXmlNode *node);
	int cmp(CXmlNode* b);
	static bool sortPred(ChildArray_t::value_type a, ChildArray_t::value_type b);
	bool SortMe();
};

class CXmlTree
{
	friend class CXmlNode;

public:
	CVSAPI_EXPORT CXmlTree();
	CVSAPI_EXPORT virtual ~CXmlTree();

	CVSAPI_EXPORT CXmlNode *ReadXmlFile(FILE* file);
	CVSAPI_EXPORT CXmlNode *ReadXmlFile(FILE* file, std::vector<cvs::string>& ignore_tag);
	CVSAPI_EXPORT CXmlNode *ParseXmlFromMemory(const char *data);
	CVSAPI_EXPORT CXmlNode *ParseXmlFromMemory(const char *data,std::vector<cvs::string>& ignore_tag);

protected:
	CCodepage m_cp;
	CXmlNode* m_lastNode;
	int m_ignore_level;
#ifdef INC_EXPAT
	XML_Parser m_parser;
#else
	void *m_parser;
#endif
	std::vector<cvs::string> m_ignore_tag;

#ifdef INC_EXPAT
	static void startElement(void *userData, const char *name, const char **atts);
	static void endElement(void *userData, const char *name);
	static void charData(void *userData, const char * s, int len);
	static int getEncoding(void *userData, const XML_Char *name, XML_Encoding *info);
#endif
};

#endif
