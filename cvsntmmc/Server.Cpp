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
#include <stdio.h>
#include "Server.h"

#ifndef _DEBUG
#define VERIFY(s) (s)
#else
#define VERIFY(s) _ASSERTE(s)
#endif

const GUID CCvsntServer::thisGuid =  { 0x89ed5344, 0x82b2, 0x4f05, { 0xb2, 0xf3, 0x43, 0x54, 0x75, 0xa1, 0x2f, 0x15 } };
const GUID CRootListFolder::thisGuid =  { 0xa896ca74, 0x5232, 0x4f66, { 0xa0, 0x98, 0x3f, 0xc3, 0x83, 0x10, 0x9, 0xd5 } };
const GUID CProtocolsFolder::thisGuid = { 0xd38ba73, 0xce9e, 0x40e9, { 0xa9, 0xa0, 0xb6, 0xf, 0xb7, 0x51, 0xee, 0x1b } };
const GUID CRootList::thisGuid = { 0xd0453bf0, 0x864d, 0x49ac, { 0x80, 0x8f, 0x97, 0x4c, 0x57, 0xe0, 0xae, 0xf1 } };
const GUID CProtocols::thisGuid = { 0x8a3f24b8, 0x61e6, 0x4882, { 0x92, 0xfd, 0xe, 0x3e, 0x37, 0x27, 0x8d, 0x94 } };
const GUID CTriggersFolder::thisGuid  = { 0x17491c08, 0x49bc, 0x44b4, { 0x9b, 0x8b, 0xf5, 0xfc, 0x48, 0x39, 0x2c, 0xc5 } };
const GUID CTriggers::thisGuid = { 0xfc3addab, 0x84b4, 0x4400, { 0x8c, 0xaa, 0x30, 0x74, 0x5b, 0x15, 0x17, 0x4a } };

//==============================================================
//
// CCvsntServer implementation
//
//
CCvsntServer::CCvsntServer(const TCHAR *szName)
{
	m_szName = szName;

	children.push_back(new CRootListFolder);
	children.push_back(new CProtocolsFolder);
	children.push_back(new CTriggersFolder);
}

CCvsntServer::~CCvsntServer()
{
    for (size_t n = 0; n < children.size(); n++)
        delete children[n];
}

HRESULT CCvsntServer::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{
    SCOPEDATAITEM sdi;

    if (!bExpanded) {
        // create the child nodes, then expand them
        for (size_t n = 0; n < children.size(); n++) {
            ZeroMemory(&sdi, sizeof(SCOPEDATAITEM) );
            sdi.mask = SDI_STR       |   // Displayname is valid
                SDI_PARAM     |   // lParam is valid
                SDI_IMAGE     |   // nImage is valid
                SDI_OPENIMAGE |   // nOpenImage is valid
                SDI_PARENT    |   // relativeID is valid
                SDI_CHILDREN;     // cChildren is valid

            sdi.relativeID  = (HSCOPEITEM)parent;
            sdi.nImage      = children[n]->GetBitmapIndex(false);
            sdi.nOpenImage  = children[n]->GetBitmapIndex(true);
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM)children[n];       // The cookie
            sdi.cChildren   = 0;

            HRESULT hr = pConsoleNameSpace->InsertItem( &sdi );

            _ASSERT( SUCCEEDED(hr) );

            children[n]->SetScopeItemValue(sdi.ID);
        }
    }

    return S_OK;
}

CRootListFolder::CRootListFolder()
{
    for (size_t n = 0; n < 20; n++) {
        children.push_back(new CRootList(n + 1));
    }
}

CRootListFolder::~CRootListFolder()
{
    for (size_t n = 0; n < children.size(); n++)
            delete children[n];
}

HRESULT CRootListFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;

    if (bShow) {
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 1, L"Directory           ", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 2, L"Published", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 3, L"Default", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 4, L"Description                ", 0, MMCLV_AUTO ))==S_OK);

        // insert items here
        RESULTDATAITEM rdi;

        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if (!bExpanded) {
            // create the child nodes, then expand them
            for (size_t n = 0; n < children.size(); n++) {
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
                rdi.mask       = RDI_STR       |   // Displayname is valid
                    RDI_IMAGE     |
                    RDI_PARAM;        // nImage is valid

                rdi.nImage      = children[n]->GetBitmapIndex(false);
                rdi.str         = MMC_CALLBACK;
                rdi.nCol        = 0;
                rdi.lParam      = (LPARAM)children[n];

                hr = pResultData->InsertItem( &rdi );
                _ASSERT( SUCCEEDED(hr) );
            }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

CProtocolsFolder::CProtocolsFolder()
{
    for (size_t n = 0; n < 5; n++) 
        children.push_back(new CProtocols(n + 1));
}

CProtocolsFolder::~CProtocolsFolder()
{
    for (size_t n = 0; n < children.size(); n++)
            delete children[n];
}

HRESULT CProtocolsFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;

    if (bShow) {
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 1, L"Description       ", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 2, L"Enabled", 0, MMCLV_AUTO ))==S_OK);
        _ASSERT( S_OK == hr );

        // insert items here
        RESULTDATAITEM rdi;

        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if (!bExpanded) {
            // create the child nodes, then expand them
            for (size_t n = 0; n < children.size(); n++) 
			{
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
                rdi.mask       = RDI_STR       |   // Displayname is valid
                    RDI_IMAGE     |
                    RDI_PARAM;        // nImage is valid

                rdi.nImage      = children[n]->GetBitmapIndex(false);
                rdi.str         = MMC_CALLBACK;
                rdi.nCol        = 0;
                rdi.lParam      = (LPARAM)children[n];

                hr = pResultData->InsertItem( &rdi );
                _ASSERT( SUCCEEDED(hr) );
           }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

CTriggersFolder::CTriggersFolder()
{
    for (size_t n = 0; n < 5; n++) 
        children.push_back(new CTriggers(n + 1));
}

CTriggersFolder::~CTriggersFolder()
{
    for (size_t n = 0; n < children.size(); n++)
            delete children[n];
}

HRESULT CTriggersFolder::OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem)
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;

    if (bShow) {
        hr = pConsole->QueryInterface(IID_IHeaderCtrl, (void **)&pHeaderCtrl);
        _ASSERT( SUCCEEDED(hr) );

        hr = pConsole->QueryInterface(IID_IResultData, (void **)&pResultData);
        _ASSERT( SUCCEEDED(hr) );

        // Set the column headers in the results pane
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 0, L"Name                ", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 1, L"Description       ", 0, MMCLV_AUTO ))==S_OK);
        VERIFY( (hr = pHeaderCtrl->InsertColumn( 2, L"Enabled", 0, MMCLV_AUTO ))==S_OK);

        // insert items here
        RESULTDATAITEM rdi;

        hr = pResultData->DeleteAllRsltItems();
        _ASSERT( SUCCEEDED(hr) );

        if (!bExpanded) {
            // create the child nodes, then expand them
            for (size_t n = 0; n < children.size(); n++) 
			{
                ZeroMemory(&rdi, sizeof(RESULTDATAITEM) );
                rdi.mask       = RDI_STR       |   // Displayname is valid
                    RDI_IMAGE     |
                    RDI_PARAM;        // nImage is valid

                rdi.nImage      = children[n]->GetBitmapIndex(false);
                rdi.str         = MMC_CALLBACK;
                rdi.nCol        = 0;
                rdi.lParam      = (LPARAM)children[n];

                hr = pResultData->InsertItem( &rdi );
                _ASSERT( SUCCEEDED(hr) );
           }
        }

        pHeaderCtrl->Release();
        pResultData->Release();
    }

    return hr;
}

const _TCHAR *CRootList::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

	if(nCol==0)
		_stprintf(buf, _T("Repository root #%d"), id);
	else
		_stprintf(buf, _T("data"));

    return buf;
}

const _TCHAR *CProtocols::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

	if(nCol==0)
		_stprintf(buf, _T("Protocols #%d"), id);
	else
		_stprintf(buf, _T("data"));

    return buf;
}

const _TCHAR *CTriggers::GetDisplayName(int nCol)
{
    static _TCHAR buf[128];

	if(nCol==0)
		_stprintf(buf, _T("Triggers #%d"), id);
	else
		_stprintf(buf, _T("data"));

    return buf;
}

