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

#include "StatNode.h"

#include "Server.h"
#include "ServerStatus.h"

// {893F5485-9BA4-4f99-85FC-C700D8DAAAF7}
const GUID CStaticNode::thisGuid = { 0x893f5485, 0x9ba4, 0x4f99, { 0x85, 0xfc, 0xc7, 0x0, 0xd8, 0xda, 0xaa, 0xf7 } };

//==============================================================
//
// CStaticNode implementation
//
//
CStaticNode::CStaticNode()
{
    children.push_back(new CServerStatus);
    children.push_back(new CCvsntServer(_T("localhost")));
}

CStaticNode::~CStaticNode()
{
    for (size_t n = 0; n < children.size(); n++)
            delete children[n];
}

HRESULT CStaticNode::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{

    //cache static node's HSCOPEITEM for future use
    m_hParentHScopeItem = parent;

    SCOPEDATAITEM sdi;

    if (!bExpanded) {
        // create the child nodes, then expand them
        for (size_t n = 0; n < children.size(); n++)
		{
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR       |   // Displayname is valid
                SDI_PARAM     |   // lParam is valid
                SDI_IMAGE     |   // nImage is valid
                SDI_OPENIMAGE |   // nOpenImage is valid
                SDI_PARENT	  |
                SDI_CHILDREN;

            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex(false);
            sdi.nOpenImage  = children[n]->GetBitmapIndex(true);
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = (n == 0); // only the first child has children

            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );
            _ASSERT( SUCCEEDED(hr) );
                        
            children[n]->SetScopeItemValue(sdi.ID);


        }
    }

    return S_OK;
}
