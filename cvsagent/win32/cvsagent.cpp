/*	cvsnt agent
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
// cvsagent.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "cvsagent.h"
#include "aboutdialog.h"
#include "listenserver.h"
#include "PasswordDialog.h"
#define CVSTOOLS_EXPORT
#include "../../cvstools/Scramble.h"

class CAgentWnd : public CWnd
{
public:
	CAgentWnd(bool u3)
	{
		m_hSession = m_hCallback = m_hDevice = NULL;
		m_dapiAvailable = false;
		m_bStorePassword = u3?true:false;
		m_bTopmost = false;
		Register();
	}

	void ShowContextMenu();

	afx_msg LRESULT OnTrayMessage(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPasswordMessage(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPasswordMessageDelete(WPARAM wParam,LPARAM lParam);
	afx_msg void OnAbout();
	afx_msg void OnQuit();
	afx_msg void OnClearPasswords();
	afx_msg void OnAlwaysOnTop();
	afx_msg void OnStorePasswords();

	DECLARE_MESSAGE_MAP();

protected:
	std::map<std::string,std::string> m_Passwords;

	bool m_bTopmost;
	bool m_dapiAvailable;
	bool m_bStorePassword;
	HSESSION m_hSession;
	HCALLBACK m_hCallback;
	HDEVICE m_hDevice;

	bool Register();
	bool Unregister();

	static void CALLBACK _dapiCallback(HDEVICE hDev, DWORD eventType, void* pEx);
	void dapiCallback(HDEVICE hDev, DWORD eventType);
};

BEGIN_MESSAGE_MAP(CAgentWnd,CWnd)
ON_MESSAGE(TRAY_MESSAGE,OnTrayMessage)
ON_MESSAGE(PWD_MESSAGE,OnPasswordMessage)
ON_MESSAGE(PWD_MESSAGE_DELETE,OnPasswordMessageDelete)
ON_COMMAND(ID_ABOUT,OnAbout)
ON_COMMAND(ID_QUIT,OnQuit)
ON_COMMAND(ID_CLEARPASSWORDS,OnClearPasswords)
ON_COMMAND(ID_ALWAYSONTOP,OnAlwaysOnTop)
ON_COMMAND(ID_STOREPASSWORDS,OnStorePasswords)
END_MESSAGE_MAP()

class CAgentApp : public CWinApp
{
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();

	static DWORD WINAPI _ThreadProc(LPVOID lpParam);
	DWORD ThreadProc();

	CActivateManifest *m_manifest;
};

CAgentApp app;

BOOL CAgentApp::InitInstance()
{
	bool bU3 = false;

	TCHAR *p = _tcsrchr(GetCommandLine(),' ');
	if(p && !_tcscmp(p+1,_T("-stop")))
	{
		CWnd *pWnd = CWnd::FindWindow(NULL,_T("_CvsAgent"));
		while(pWnd)
		{
			pWnd->SendMessage(WM_COMMAND,ID_QUIT);
			Sleep(500);
			pWnd = CWnd::FindWindow(NULL,_T("_CvsAgent"));
		}
		return FALSE;
	}
	else if(p && !_tcscmp(p+1,_T("-u3")))
		bU3 = true;
	else if(p && !_tcscmp(p+1,_T("-uninstall")))
		return FALSE;

	CWnd *pWnd = CWnd::FindWindow(NULL,_T("_CvsAgent"));
	if(pWnd)
		return FALSE;

	InitCommonControls();
	m_manifest = new CActivateManifest;

	m_pMainWnd = new CAgentWnd(bU3);
	m_pMainWnd->CreateEx(0,_T("static"),_T("_CvsAgent"),WS_OVERLAPPED,CRect(1,1,1,1),NULL,0);

	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
	nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
	nid.uCallbackMessage=TRAY_MESSAGE;
	nid.hIcon=LoadIcon(IDI_CVSAGENT);
	nid.uID=IDI_CVSAGENT;
	_tcscpy(nid.szTip,_T("CVSNT password agent"));
	nid.hWnd=m_pMainWnd->GetSafeHwnd();
	Shell_NotifyIcon(NIM_ADD,&nid);

	CloseHandle(::CreateThread(0,0,_ThreadProc,this,0,NULL));
	return TRUE;
}

DWORD WINAPI CAgentApp::_ThreadProc(LPVOID lpParam)
{
	return ((CAgentApp*)lpParam)->ThreadProc();
}

DWORD CAgentApp::ThreadProc()
{
	CListenServer l;
	l.Listen("32401");
	return 0;
}

BOOL CAgentApp::ExitInstance()
{
	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
	nid.hWnd=m_pMainWnd->GetSafeHwnd();
	nid.uID=IDI_CVSAGENT;
	Shell_NotifyIcon(NIM_DELETE,&nid);
	delete m_manifest;
	return TRUE;
}

LRESULT CAgentWnd::OnTrayMessage(WPARAM wParam, LPARAM lParam)
{
	switch(lParam)
	{
	case WM_LBUTTONDBLCLK:
		break;
	case WM_RBUTTONDOWN:
	case WM_CONTEXTMENU:
		ShowContextMenu();
		break;
	}
	
	return 0;
}

void CAgentWnd::ShowContextMenu()
{
	CMenu menu;
	CPoint pt;
	menu.LoadMenu(IDR_MENU1);
	SetForegroundWindow();
	GetCursorPos(&pt);
	CMenu *sub = menu.GetSubMenu(0);
	sub->CheckMenuItem(ID_ALWAYSONTOP,m_bTopmost?MF_CHECKED:MF_UNCHECKED);
	sub->EnableMenuItem(ID_STOREPASSWORDS,m_dapiAvailable?MF_ENABLED:MF_GRAYED);
	sub->CheckMenuItem(ID_STOREPASSWORDS,m_bStorePassword?MF_CHECKED:MF_UNCHECKED);
	sub->TrackPopupMenu(TPM_BOTTOMALIGN|TPM_CENTERALIGN|TPM_LEFTBUTTON,pt.x,pt.y,this);
}

void CAgentWnd::OnAbout()
{
	CAboutDialog dlg;
	dlg.DoModal();
}

void CAgentWnd::OnQuit()
{
	PostQuitMessage(0);
}

void CAgentWnd::OnClearPasswords()
{
	m_Passwords.clear();
	if(m_dapiAvailable && m_bStorePassword)
	{
		dapiDeleteCookie(m_hDevice,L"CvsAgent",NULL);
		BYTE av = m_bStorePassword?1:0;
		HRESULT res = dapiWriteBinaryCookie(m_hDevice,L"CvsAgent",L"_StorePasswords",&av,1);
	}
}

void CAgentWnd::OnAlwaysOnTop()
{
	m_bTopmost=!m_bTopmost;
}

void CAgentWnd::OnStorePasswords()
{
	m_bStorePassword=!m_bStorePassword;
	if(m_dapiAvailable)
	{
		BYTE av = m_bStorePassword?1:0;
		HRESULT res = dapiWriteBinaryCookie(m_hDevice,L"CvsAgent",L"_StorePasswords",&av,1);
	}
}

LRESULT CAgentWnd::OnPasswordMessage(WPARAM wParam,LPARAM lParam)
{
	char *buffer = (char*)wParam;
	int *len = (int*)lParam;
	CScramble scramble;

	if(m_Passwords.find(buffer)!=m_Passwords.end())
	{
		*len = m_Passwords[buffer].size();
		strcpy(buffer,m_Passwords[buffer].c_str());
		return 1;
	}

	if(m_dapiAvailable && m_bStorePassword)
	{
		BYTE buf[BUFSIZ];
		DWORD bufLen = sizeof(buf);
		if(dapiReadBinaryCookie(m_hDevice,L"CvsAgent",cvs::wide(buffer),buf,&bufLen)==S_OK)
		{
			*len = (int)bufLen;
			memcpy(buffer,buf,bufLen);
			buffer[bufLen]='\0';
			return 1;
		}
	}

	CPasswordDialog dlg(m_bTopmost);
	dlg.m_szCvsRoot=buffer;
	if(dlg.DoModal()!=IDOK)
		return 0;
	strcpy(buffer,scramble.Scramble(dlg.m_szPassword.c_str()));
	*len=(int)strlen(buffer);

	if(m_dapiAvailable && m_bStorePassword)
	{
		dapiWriteBinaryCookie(m_hDevice, L"CvsAgent", cvs::wide(dlg.m_szCvsRoot.c_str()), (BYTE*)buffer, *len);
	}
	else
		m_Passwords[dlg.m_szCvsRoot]=buffer;

	return 1;
}

LRESULT CAgentWnd::OnPasswordMessageDelete(WPARAM wParam,LPARAM lParam)
{
	char *buffer = (char*)wParam;

	if(m_Passwords.find(buffer)!=m_Passwords.end())
		m_Passwords.erase(m_Passwords.find(buffer));

	if(m_dapiAvailable && m_bStorePassword)
	{
		dapiDeleteCookie(m_hDevice, L"CvsAgent", cvs::wide(buffer));
	}
	return 0;
}

bool CAgentWnd::Register()
{
	m_dapiAvailable = false;

	__try
	{
		if(dapiCreateSession(&m_hSession)==S_OK)
		{
			if(dapiRegisterCallback(m_hSession, _T(""), _dapiCallback, this, &m_hCallback)!=S_OK)
				Unregister();
		}
		else
			Unregister();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

    return true;
}

bool CAgentWnd::Unregister()
{
	__try
	{
		if(m_hCallback)
			dapiUnregisterCallback(m_hCallback);
		if(m_hSession)
			dapiDestroySession(m_hSession);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	m_hCallback=m_hSession=NULL;
	m_dapiAvailable = false;
	return m_dapiAvailable;
}

void CAgentWnd::_dapiCallback(HDEVICE hDev, DWORD eventType, void* pEx)
{
	((CAgentWnd*)pEx)->dapiCallback(hDev,eventType);
}

void CAgentWnd::dapiCallback(HDEVICE hDev, DWORD eventType)
{
	switch(eventType)
	{
		case DAPI_EVENT_DEVICE_CONNECT:
		case DAPI_EVENT_DEVICE_RECONNECT:
		case DAPI_EVENT_DEVICE_NEW_CONFIG:
			m_dapiAvailable = false;
			if(dapiQueryDeviceCapability(hDev, DAPI_CAP_U3_COOKIE)==S_OK)
			{
				BYTE av;
				DWORD l = 1;

				m_dapiAvailable = true;
				m_bStorePassword = false;
				if(dapiReadBinaryCookie(hDev,L"CvsAgent",L"_StorePasswords",&av,&l)==S_OK)
					m_bStorePassword=av?true:false;
			}
			m_hDevice = hDev;
			break;
		case DAPI_EVENT_DEVICE_DISCONNECT:
			m_dapiAvailable = false;
			m_hDevice = NULL;
			break;
	}
}
