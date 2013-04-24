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

#ifndef _LAND_H
#define _LAND_H

#include "DeleBase.h"

class CServerStatus : public CDelegationBase 
{
public:
    static const GUID thisGuid;

    CServerStatus() { }

    virtual ~CServerStatus() {}

    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Server Status"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex(bool open) { return INDEX_CLOSEDFOLDER; }

    virtual void SetScopeItemValue(LONG scopeitem) { m_hParentHScopeItem = scopeitem; }
    virtual HSCOPEITEM GetParentScopeItem() { return m_hParentHScopeItem; }

private:
    enum { IDM_NEW_LAND = 2 };

    HSCOPEITEM m_hParentHScopeItem;
};


#endif // _LAND_H
