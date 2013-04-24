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
// WorkspaceViewer.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WorkspaceViewer.h"
#include "MainFrm.h"

#include "WorkspaceViewerDoc.h"
#include "WorkspaceViewerView.h"

#include "StatLink.h"

#include "../version.h"
#include "afxwin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool g_splash;

// CWorkspaceViewerApp

BEGIN_MESSAGE_MAP(CWorkspaceViewerApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CWorkspaceViewerApp construction

CWorkspaceViewerApp::CWorkspaceViewerApp()
{
}

// The one and only CWorkspaceViewerApp object

CWorkspaceViewerApp theApp;

// CWorkspaceViewerApp initialization

BOOL CWorkspaceViewerApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	m_manifest = new CActivateManifest;

	WSADATA wsa;
	WSAStartup(MAKEWORD(2,0),&wsa);

	// Initialize OLE 2.0 libraries
	if (!AfxOleInit ())
	{
		AfxMessageBox (TEXT ("Unable to load OLE 2.0 libraries!"));
		return (FALSE);
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("March-Hare Software Ltd"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWorkspaceViewerDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CWorkspaceViewerView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate); 
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The one and only window has been initialized, so show and update it
	m_pMainWnd->SetWindowText(_T("Workspace Viewer"));
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CStaticLink m_stWorkmgr;
	CStaticLink m_stMoreInfo;
	CStaticLink m_stBuyNow;
	int m_nCountdown;
	UINT_PTR m_nTimer;
	afx_msg void OnBnClickedOk();
	afx_msg void OnTimer(UINT nIDEvent);
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MARCHHARE, m_stWorkmgr);
	DDX_Control(pDX, IDC_MOREINFO, m_stMoreInfo);
	DDX_Control(pDX, IDC_BUYNOW, m_stBuyNow);
}

BOOL CAboutDlg::OnInitDialog()
{
	TCHAR tmp[64];
	CDialog::OnInitDialog();
	SetDlgItemTextA(m_hWnd,IDC_WMVERSION,"Workspace Viewer "CVSNT_PRODUCTVERSION_STRING);
	m_stMoreInfo.m_link="http://march-hare.com/cvsnt/features/workmgr/";
	m_stBuyNow.m_link="http://store.march-hare.com/s.nl?sc=2&category=2";
	if(g_splash)
	{
		SetTimer(m_nTimer,1000,NULL);
		m_nCountdown=30;
		_sntprintf(tmp,sizeof(tmp),_T("OK - %d"),m_nCountdown);
		SetDlgItemText(IDOK,tmp);
	}
	else
		m_nTimer = 0;
	return TRUE;
}

void CAboutDlg::OnBnClickedOk()
{
	if(m_nTimer)
		KillTimer(m_nTimer);
	OnOK();
}

void CAboutDlg::OnTimer(UINT nIDEvent)
{
	TCHAR tmp[64];

	m_nCountdown--;
	if(!m_nCountdown)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
		PostMessage(WM_COMMAND,IDOK);
	}
	_sntprintf(tmp,sizeof(tmp),_T("OK - %d"),m_nCountdown);
	SetDlgItemText(IDOK,tmp);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// App command to run the dialog
void CWorkspaceViewerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BOOL CWorkspaceViewerApp::ExitInstance()
{
	delete m_manifest;
	return TRUE;
}

