// ViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WorkspaceViewer.h"
#include "ViewDlg.h"


// CViewDlg dialog

IMPLEMENT_DYNAMIC(CViewDlg, CDialog)
CViewDlg::CViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CViewDlg::IDD, pParent)
{
}

CViewDlg::~CViewDlg()
{
}

void CViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_edText);
}


BEGIN_MESSAGE_MAP(CViewDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CViewDlg message handlers

BOOL CViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CFileAccess acc;
	if(acc.open(m_szFile.c_str(),"r"))
	{
		cvs::string str;
		str.resize((size_t)acc.length());
		acc.read((char*)str.data(),str.size());
		::SetWindowTextA(m_edText.m_hWnd,str.c_str());
		m_edText.SetSel(-1,-1);
		acc.close();

		SetWindowText(cvs::wide(m_szTitle.c_str()));
	}
	
	return TRUE;  
}

void CViewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_edText.m_hWnd)
		m_edText.MoveWindow(0,0,cx,cy);
}
