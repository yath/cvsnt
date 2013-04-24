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

#ifndef _SNAPINBASE_H
#define _SNAPINBASE_H

#include <vector>

#include "DeleBase.h"

class CStaticNode : public CDelegationBase
{
public:
    static const GUID thisGuid;

    CStaticNode();

    virtual ~CStaticNode();

    virtual const _TCHAR *GetDisplayName(int nCol = 0)
	{
		return _T("CVSNT Service Configuration");
    }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_NONE; }
    virtual void SetScopeItemValue(HSCOPEITEM hscopeitem) { m_hParentHScopeItem = hscopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);

private:
	std::vector<CDelegationBase *>children;

    HSCOPEITEM m_hParentHScopeItem;
};



#endif // _SNAPINBASE_H
