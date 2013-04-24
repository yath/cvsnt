//==============================================================;
//
//      This source code is only intended as a supplement to
//  existing Microsoft documentation.
//
//
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 - 2001 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _PEOPLE_H
#define _PEOPLE_H

#include "DeleBase.h"

#include <vector>

class CRootList : public CDelegationBase
{
public:
    static const GUID thisGuid;

	CRootList(int i) : id(i) { }
    virtual ~CRootList() {}

    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }

private:       
    int id;
};

class CRootListFolder : public CDelegationBase
{
public:
    static const GUID thisGuid;

	CRootListFolder();
    virtual ~CRootListFolder();

    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Repository roots"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }
    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }
        
public:
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);

private:
    HSCOPEITEM m_hParentHScopeItem;

	std::vector<CDelegationBase *> children;
};

class CProtocols : public CDelegationBase
{
public:
    CProtocols(int i) : id(i) { }
    virtual ~CProtocols() {}

    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }

private:
    static const GUID thisGuid;

    int id;
};

class CProtocolsFolder : public CDelegationBase
{
public:
    static const GUID thisGuid;

	CProtocolsFolder();
    virtual ~CProtocolsFolder();

    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Protocols"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }
    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

public:
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);

private:
    HSCOPEITEM m_hParentHScopeItem;

	std::vector<CDelegationBase *> children;
};

class CTriggers : public CDelegationBase
{
public:
    CTriggers(int i) : id(i) { }
    virtual ~CTriggers() {}

    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }

private:
    static const GUID thisGuid;

    int id;
};

class CTriggersFolder : public CDelegationBase
{
public:
    static const GUID thisGuid;

	CTriggersFolder();
    virtual ~CTriggersFolder();

    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Triggers"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }
    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

public:
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);

private:
    HSCOPEITEM m_hParentHScopeItem;

	std::vector<CDelegationBase *> children;
};

class CCvsntServer : public CDelegationBase
{
public:
    static const GUID thisGuid;

    CCvsntServer(const TCHAR *szName);
    virtual ~CCvsntServer();

    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return m_szName; }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }

    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);

    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

private:
	const TCHAR *m_szName;

    enum { IDM_NEW_PEOPLE = 1 };

    HSCOPEITEM m_hParentHScopeItem;

	std::vector<CDelegationBase *> children;
};

#endif // _PEOPLE_H
