/*	Workspace Viewer
    Copyright (C) 2004-5 Tony Hoyle and March-Hare Software Ltd

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
// WorkspaceViewerDoc.cpp : implementation of the CWorkspaceViewerDoc class
//

#include "stdafx.h"
#include "WorkspaceViewer.h"

#include "WorkspaceViewerDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWorkspaceViewerDoc

IMPLEMENT_DYNCREATE(CWorkspaceViewerDoc, CDocument)

BEGIN_MESSAGE_MAP(CWorkspaceViewerDoc, CDocument)
END_MESSAGE_MAP()


// CWorkspaceViewerDoc construction/destruction

CWorkspaceViewerDoc::CWorkspaceViewerDoc()
{
	// TODO: add one-time construction code here

}

CWorkspaceViewerDoc::~CWorkspaceViewerDoc()
{
}

BOOL CWorkspaceViewerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CWorkspaceViewerDoc serialization

void CWorkspaceViewerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CWorkspaceViewerDoc diagnostics

#ifdef _DEBUG
void CWorkspaceViewerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWorkspaceViewerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CWorkspaceViewerDoc commands
