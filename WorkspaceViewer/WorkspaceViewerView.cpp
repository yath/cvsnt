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
// WorkspaceViewerView.cpp : implementation of the CWorkspaceViewerView class
//

#include "stdafx.h"
#include "WorkspaceViewer.h"

#include "WorkspaceViewerDoc.h"
#include "WorkspaceViewerView.h"
#include "ViewDlg.h"
#include ".\workspaceviewerview.h"

#define SERVICE "_cvspserver._tcp"

extern bool g_splash;

CWorkspaceViewerView::servers_t CWorkspaceViewerView::m_servers;

// CWorkspaceViewerView

IMPLEMENT_DYNCREATE(CWorkspaceViewerView, CTreeView)

BEGIN_MESSAGE_MAP(CWorkspaceViewerView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnTvnDeleteitem)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
	ON_COMMAND(ID_VIEW_FILES, OnViewFiles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FILES, OnUpdateViewFiles)
	ON_COMMAND(ID_VIEW_TAGS, OnViewTags)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TAGS, OnUpdateViewTags)
	ON_COMMAND(ID_VIEW_BRANCHES, OnViewBranches)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BRANCHES, OnUpdateViewBranches)
	ON_COMMAND(ID_NEW, OnNew)
	ON_UPDATE_COMMAND_UI(ID_NEW, OnUpdateNew)
	ON_COMMAND(ID_VIEW_REVISIONS, OnViewRevisions)
	ON_UPDATE_COMMAND_UI(ID_VIEW_REVISIONS, OnUpdateViewRevisions)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWGLOBALSERVERS, OnUpdateViewShowglobalservers)
	ON_COMMAND(ID_VIEW_SHOWGLOBALSERVERS, OnViewShowglobalservers)
	ON_UPDATE_COMMAND_UI(ID_MENU_VIEW, OnUpdateMenuView)
	ON_COMMAND(ID_MENU_VIEW, OnMenuView)
	ON_UPDATE_COMMAND_UI(ID_MENU_PROPERTIES, OnUpdateMenuProperties)
	ON_COMMAND(ID_MENU_PROPERTIES, OnMenuProperties)
	ON_UPDATE_COMMAND_UI(ID_MENU_DELETE, OnUpdateMenuDelete)
	ON_COMMAND(ID_MENU_DELETE, OnMenuDelete)
END_MESSAGE_MAP()

// CWorkspaceViewerView construction/destruction

CWorkspaceViewerView::CWorkspaceViewerView()
{
	m_bViewFiles = false;
	m_bViewTags = false;
	m_bViewBranches = true;
	m_bViewRevisions = false;
	m_bShowGlobal = true;
}

CWorkspaceViewerView::~CWorkspaceViewerView()
{
}

BOOL CWorkspaceViewerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style|=TVS_HASBUTTONS|TVS_HASLINES|TVS_EDITLABELS;

	return CTreeView::PreCreateWindow(cs);
}


void CWorkspaceViewerView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();
	char tmp[32];

	if(!CGlobalSettings::GetUserValue("WorkspaceViewer",NULL,"ViewFiles",tmp,sizeof(tmp)))
		m_bViewFiles = atoi(tmp)?true:false;

	if(!CGlobalSettings::GetUserValue("WorkspaceViewer",NULL,"ViewTags",tmp,sizeof(tmp)))
		m_bViewTags = atoi(tmp)?true:false;

	if(!CGlobalSettings::GetUserValue("WorkspaceViewer",NULL,"ViewBranches",tmp,sizeof(tmp)))
		m_bViewBranches = atoi(tmp)?true:false;

	if(!CGlobalSettings::GetUserValue("WorkspaceViewer",NULL,"ViewRevisions",tmp,sizeof(tmp)))
		m_bViewRevisions = atoi(tmp)?true:false;

	CImageList il;
	il.Create(16,16,ILC_COLOR8|ILC_MASK,5,5);
	il.Add(AfxGetApp()->LoadIcon(IDI_NETHOOD));
	il.Add(AfxGetApp()->LoadIcon(IDI_SERVER));
	il.Add(AfxGetApp()->LoadIcon(IDI_FLDRCLOSED));
	il.Add(AfxGetApp()->LoadIcon(IDI_FLDROPEN));
	il.Add(AfxGetApp()->LoadIcon(IDI_DOCUMENT));
	il.Add(AfxGetApp()->LoadIcon(IDI_BRANCH));
	il.Add(AfxGetApp()->LoadIcon(IDI_WORLD));
	GetTreeCtrl().SetImageList(&il,TVSIL_NORMAL);
	il.Detach();

	Refresh();

	int nSeen = 0;
	CGlobalSettings::GetUserValue("WorkspaceViewer",NULL,"SplashSeen",nSeen);
	if(!nSeen)
	{
		g_splash = true;
		AfxGetMainWnd()->PostMessage(WM_COMMAND,ID_APP_ABOUT);
		CGlobalSettings::SetUserValue("WorkspaceViewer",NULL,"SplashSeen",1);
	}
}

void CWorkspaceViewerView::Refresh()
{
	GetTreeCtrl().DeleteAllItems();

	HTREEITEM hRoot = GetTreeCtrl().InsertItem(_T("Local Servers"),0,0);

	int nResp;
	CMdnsHelperBase::mdnsType eResp = CMdnsHelperBase::mdnsDefault;

  	if(!CGlobalSettings::GetGlobalValue("cvsnt","PServer","ResponderType",nResp))
	{
		switch(nResp)
		{
		case 0:
			AfxMessageBox(_T("mdns is disabled on this host!"));
			return;
		case 1:
			eResp = CMdnsHelperBase::mdnsMini;
			break;
		case 2:
			eResp = CMdnsHelperBase::mdnsApple;
			break;
		case 3:
			eResp = CMdnsHelperBase::mdnsHowl;
			break;
		default:
			eResp = CMdnsHelperBase::mdnsMini;
			break;
		}
	}
	CZeroconf zc(eResp, CGlobalSettings::GetLibraryDirectory(CGlobalSettings::GLDMdns));
	const CZeroconf::server_struct_t *serv;
	zc.BrowseForService(SERVICE,CZeroconf::zcAddress);
	bool first = true;
	while((serv=zc.EnumServers(first))!=NULL)
	{
		ServerConnectionInfo *srv = new ServerConnectionInfo;
		srv->level=0;
		srv->user=false;

		srv->server = serv->server;
		srv->server.resize(srv->server.find_first_of('.'));
		cvs::sprintf(srv->port,32,"%u",serv->port);
		srv->srvname = serv->servicename;
		HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(serv->servicename.c_str()),1,1,hRoot);
		GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
		GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
	}

	int n =0;
	char key[256];
	char val[256];
	while(!CGlobalSettings::EnumUserValues("WorkspaceManager","Servers",n++,key,sizeof(key),val,sizeof(val)))
	{
		CRootSplitter split;

		if(strncmp(key,"Serv_",5))
			continue;
		if(val[0]!=':' && !strchr(val,'/'))
		{
			char *p=strchr(val,':');
			if(p) *p='\0';
			split.m_server = val;
			if(p) split.m_port = p+1;
			else split.m_port="";
		}
		else if(!split.Split(val))
			continue;

		if(!strcmp(split.m_protocol.c_str(),"ftp"))
			continue;

		ServerConnectionInfo *srv = new ServerConnectionInfo;
		srv->level=0;
		srv->user = true;
		srv->root = split.m_root;
		srv->protocol = split.m_protocol;
		srv->keywords = split.m_keywords;
		srv->username = split.m_username;
		srv->password = split.m_password;
		srv->server = split.m_server;
		srv->port = split.m_port;
		srv->directory = split.m_directory;
		srv->module = split.m_module;
		srv->srvname = key+5;
		HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(key+5),1,1,GetTreeCtrl().GetRootItem());
		GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
		GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
	}

	GetTreeCtrl().Expand(hRoot,TVE_EXPAND);

	if(m_bShowGlobal)
	{
		hRoot = GetTreeCtrl().InsertItem(_T("Global Directory"),6,6);
		ServerConnectionInfo *srv = new ServerConnectionInfo;
		srv->level = -2;
		GetTreeCtrl().SetItemData(hRoot,(DWORD_PTR)srv);

		CDnsApi dns;
		std::map<cvs::string,int> dupMap;

		if(dns.Lookup("_cvspserver._tcp.cvsnt.org", DNS_TYPE_PTR))
		{
			do
			{
				CDnsApi srvDns;
				const char *srvName = dns.GetRRPtr();
				const char *srvTxt = NULL, *srvPtr = NULL;
				CDnsApi::SrvRR *rr = NULL;

				if(!srvName || dupMap[srvName]++)
					continue;

				if(srvName && strchr(srvName,'.') && srvDns.Lookup(srvName,DNS_TYPE_ANY))
				{
					do
					{
						if(!srvTxt)
							srvTxt = srvDns.GetRRTxt();

						if(!rr)
							rr=srvDns.GetRRSrv();

						srvPtr = srvDns.GetRRPtr();

						if(srvPtr || (srvTxt && rr)) break;
					} while(srvDns.Next());

					if(srvPtr)
					{
						ServerConnectionInfo *srv = new ServerConnectionInfo;
						srv->level=-1;
						srv->user=false;

						srv->module = srvName;

						cvs::string title = srvName;
						title.resize(title.find_first_of('.'));
						srv->srvname = title;
						HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(title.c_str()),2,3,hRoot);
						GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
						GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
					}
					else
					{
						if(rr)
						{
							ServerConnectionInfo *srv = new ServerConnectionInfo;
							srv->level=0;
							srv->user=false;

							srv->server = rr->server;
							cvs::sprintf(srv->port,32,"%u",rr->port);

							cvs::string title = srvName;
							title.resize(title.find_first_of('.'));
							srv->srvname = title;
							HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(title.c_str()),1,1,hRoot);
							GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
							GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);

							if(srvTxt)
							{
								srv->level=1;
								srv->enumerated=false;
								srv->anonymous=false;
								srv->module="";
								srv->root = srvTxt;
							}
						}
					}
				}
			} while(dns.Next());
		}

		GetTreeCtrl().SortChildren(hRoot);
		GetTreeCtrl().Expand(hRoot,TVE_EXPAND);
	}
}

void CWorkspaceViewerView::OnTvnDeleteitem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if(pNMTreeView->itemOld.lParam)
		delete (ServerConnectionInfo*)pNMTreeView->itemOld.lParam;

	*pResult = 0;
}

bool CWorkspaceViewerView::AskForPassword(ServerConnectionInfo *dir)
{
	CBrowserPasswordDialog dlg(m_hWnd);
	return dlg.ShowDialog(dir);
}

void CWorkspaceViewerView::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	*pResult = 0;

	if(pNMTreeView->action&TVE_COLLAPSE)
	{
		if(!(pNMTreeView->action&TVE_COLLAPSERESET) && (pNMTreeView->itemNew).hItem != GetTreeCtrl().GetRootItem())
			GetTreeCtrl().Expand((pNMTreeView->itemNew).hItem,TVE_COLLAPSE|TVE_COLLAPSERESET);
		return;
	}

	UINT nMask = GetTreeCtrl().GetItemState((pNMTreeView->itemNew).hItem,TVIS_EXPANDEDONCE);
	if (nMask & TVIS_EXPANDEDONCE)
		return;

	HTREEITEM hParent = pNMTreeView->itemNew.hItem;
	ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hParent);

	if(!dir || !hParent || dir->level==-2)
		return;

	CWaitCursor wait;

	HTREEITEM hItem = GetTreeCtrl().GetChildItem(hParent);
	while(hItem)
	{
		HTREEITEM hNextItem = GetTreeCtrl().GetNextSiblingItem(hItem);
		GetTreeCtrl().DeleteItem(hItem);
		hItem = hNextItem;
	}

	if(dir->level == -1)
	{
		CDnsApi dns;
		std::map<cvs::string,int> dupMap;

		if(dns.Lookup(dir->module.c_str(), DNS_TYPE_PTR))
		{
			do
			{
				CDnsApi srvDns;
				const char *srvName = dns.GetRRPtr();
				const char *srvTxt = NULL, *srvPtr = NULL;
				CDnsApi::SrvRR *rr = NULL;

				if(!srvName || dupMap[srvName]++)
					continue;

				if(srvName && strchr(srvName,'.') && srvDns.Lookup(srvName,DNS_TYPE_ANY))
				{
					do
					{
						if(!srvTxt)
							srvTxt = srvDns.GetRRTxt();

						if(!rr)
							rr=srvDns.GetRRSrv();

						srvPtr = srvDns.GetRRPtr();

						if(srvPtr || (srvTxt && rr)) break;
					} while(srvDns.Next());

					if(srvPtr)
					{
						ServerConnectionInfo *srv = new ServerConnectionInfo;
						srv->level=-1;
						srv->user=false;

						srv->module = srvPtr;

						cvs::string title = srvName;
						title.resize(title.find_first_of('.'));
						srv->srvname = title;
						HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(title.c_str()),2,3,hParent);
						GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
						GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
					}
					else
					{
						if(rr)
						{
							ServerConnectionInfo *srv = new ServerConnectionInfo;
							srv->level=0;
							srv->user=false;

							srv->server = rr->server;
							cvs::sprintf(srv->port,32,"%u",rr->port);

							cvs::string title = srvName;
							title.resize(title.find_first_of('.'));
							srv->srvname = title;
							HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(title.c_str()),1,1,hParent);
							GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
							GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);

							if(srvTxt)
							{
								srv->level=1;
								srv->enumerated=false;
								srv->anonymous=false;
								srv->module="";
								srv->root = srvTxt;
							}
						}
					}
				}
			} while(dns.Next());
		}
		GetTreeCtrl().SortChildren(hParent);
		return;
	}

	if(!dir->level)
	{
		// Server level.  Initialise structure and get repository details.
		CServerInfo si;
		CServerInfo::remoteServerInfo rsi;
		cvs::string tempServ = dir->server + ":" + dir->port;

        if(dir->root.size() || !si.getRemoteServerInfo(tempServ.c_str(),rsi))
		{
			if(!dir->root.size())
				return;
			ServerConnectionInfo *newdir = new ServerConnectionInfo;
			*newdir = *dir;
			newdir->level = 1;
			newdir->user=false;
			newdir->enumerated=false;
			newdir->anonymous=false;
			cvs::string tmp = dir->directory;
			if(newdir->module.size())
			{
				tmp+=" (";
				tmp+=newdir->module;
				tmp+=")";
			}
			hItem=GetTreeCtrl().InsertItem(cvs::wide(tmp.c_str()),2,3,hParent);
			GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)newdir);
			GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem); 
		}
		else
		{
			bool first = true;
			for(CServerInfo::remoteServerInfo::repositories_t::const_iterator i = rsi.repositories.begin(); i!=rsi.repositories.end(); ++i)
			{
				ServerConnectionInfo *newdir = new ServerConnectionInfo;
				*newdir = *dir;
				newdir->level = 1;
				newdir->user=false;
				newdir->anonymous=false;
				newdir->enumerated=true;
				newdir->module="";
				newdir->anon_user = rsi.anon_username;
				newdir->anon_proto = rsi.anon_protocol;
				newdir->default_proto = rsi.default_protocol;
				newdir->directory = i->first;

				hItem=GetTreeCtrl().InsertItem(cvs::wide(i->second.c_str()),2,3,hParent);
				GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)newdir);
				GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
			}
		}
		return;
	}
	else if(dir->level>=1)
	{
		const char *cvsnt = CGlobalSettings::GetCvsCommand();

		m_dirList.clear();

		CServerConnection conn;
		cvs::string tmp;

		if(dir->module.size())
			cvs::sprintf(tmp,80,"--utf8 rls -qe \"%s\"",dir->module.c_str());
		else
			tmp="--utf8 rls -qe";
		if(!conn.Connect(tmp.c_str(), dir, this))
			return;

		m_tagList.clear();
		if(dir->module.size() && !dir->tag.size() && (m_bViewTags || m_bViewBranches))
		{
			CRunFile rf;
			int res;

			rf.setOutput(_BuildTagList,this);
			rf.resetArgs();
			rf.addArg(cvsnt);
			rf.addArg("--utf8");
			rf.addArg("-d");
			rf.addArg(dir->root.c_str());
			rf.addArg("rlog");
			rf.addArg("-lh");
			rf.addArg(dir->module.c_str());
			if(!rf.run(NULL))
			{
				AfxMessageBox(_T("Unable to run cvs"));
				return;
			}
			rf.wait(res);
		}

		for(std::map<cvs::string,int>::const_iterator i = m_tagList.begin(); i!=m_tagList.end(); ++i)
		{
			ServerConnectionInfo *tagdir = new ServerConnectionInfo;
			*tagdir = *dir;
			tagdir->tag = i->first;
			hItem=GetTreeCtrl().InsertItem(cvs::wide(i->first.c_str()),5,5,hParent);
			GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)tagdir);						
			GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
		}

		for(size_t n=0; n<m_dirList.size(); n++)
		{
			const char *p=m_dirList[n].c_str(),*q;
			bool isfile = false;
			if(*p!='D')
				isfile = true;
			else
				p++;

			if(isfile && !m_bViewFiles)
				continue;

			if(*(p++)!='/')
				continue;
			q=strchr(p,'/');
			if(!q)
				continue;
			cvs::filename fn;
			fn.assign(p,q-p);

			ServerConnectionInfo *newdir = new ServerConnectionInfo;
			*newdir = *dir;
			newdir->level++;
			if(newdir->module.size())
				newdir->module+="/";
			newdir->module += fn.c_str();
			newdir->isfile = isfile;

			if(m_bViewRevisions && isfile)
			{
				p=++q;
				q=strchr(p,'/');
				if(q && q>p)
				{
					fn+=" (";
					fn.append(p,q-p);
					fn+=")";
				}
			}

			hItem=GetTreeCtrl().InsertItem(cvs::wide(fn.c_str()),isfile?4:2,isfile?4:3,hParent);
			GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)newdir);
			if(!isfile)
				GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);
		}
	}
}

void CWorkspaceViewerView::ProcessOutput(const char *line)
{
	m_dirList.push_back(line);
}

int CWorkspaceViewerView::_BuildTagList(const char *data,size_t len,void *param)
{
	return ((CWorkspaceViewerView*)param)->BuildTagList(data,len);
}

int CWorkspaceViewerView::BuildTagList(const char *data,size_t len)
{
	bool tagMode = false;
	const char *p = data, *q;
	cvs::string str;
	do
	{
		for(q=p; q<data+len; q++)
			if(*q=='\n')
				break;
		if(q>p+1)
		{
			q--;
			str.assign(p,q-p);
			if(str=="symbolic names:")
				tagMode=true;
			else if(!strncmp(str.c_str(),"keyword substitution:",21))
				tagMode=false;
			else if(tagMode)
			{
				size_t pos = str.find_first_of(':');
				if(pos!=cvs::string::npos)
				{
					cvs::string tag=str.substr(pos+1);
					str.resize(pos);
					if(strstr(tag.c_str(),".0."))
					{
						if(m_bViewBranches)
							m_tagList[str]++;
					}
					else
					{
						if(m_bViewTags)
							m_tagList[str]++;
					}
				}
			}
		}
		p=q;
		while(p<data+len && isspace((unsigned char)*p))
			p++;
	} while(p<data+len);
	return (int)len;
}

void CWorkspaceViewerView::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint pos(GetMessagePos());
	GetTreeCtrl().ScreenToClient(&pos);
	HTREEITEM hItem = GetTreeCtrl().HitTest(pos);
	GetTreeCtrl().SelectItem(hItem);
	if(hItem)
	{
		GetTreeCtrl().ClientToScreen(&pos);
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		CMenu menu;
		menu.LoadMenu(IDR_LEFTMENU);
		CMenu *pop = menu.GetSubMenu(0);

		m_menuObj = dir;
		pop->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,pos.x,pos.y,AfxGetMainWnd());
	}

	*pResult = 0;
}

void CWorkspaceViewerView::OnViewFiles()
{
	m_bViewFiles=!m_bViewFiles;
	CGlobalSettings::SetUserValue("WorkspaceViewer",NULL,"ViewFiles",m_bViewFiles);
}

void CWorkspaceViewerView::OnUpdateViewFiles(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewFiles?1:0);
	pCmdUI->Enable();
}

void CWorkspaceViewerView::OnViewTags()
{
	m_bViewTags=!m_bViewTags;
	CGlobalSettings::SetUserValue("WorkspaceViewer",NULL,"ViewTags",m_bViewTags);
}

void CWorkspaceViewerView::OnUpdateViewTags(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewTags?1:0);
	pCmdUI->Enable();
}

void CWorkspaceViewerView::OnViewBranches()
{
	m_bViewBranches=!m_bViewBranches;
	CGlobalSettings::SetUserValue("WorkspaceViewer",NULL,"ViewBranches",m_bViewBranches);
}

void CWorkspaceViewerView::OnUpdateViewBranches(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewBranches?1:0);
	pCmdUI->Enable();
}

void CWorkspaceViewerView::OnNew()
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(!dir)
		{
			CBrowserRootDialog dlg(m_hWnd);
			ServerConnectionInfo *srv = new ServerConnectionInfo;
			srv->level=0;
			srv->user = true;
			if(!dlg.ShowDialog(srv,"Add user defined server",0))
			{
				delete srv;
				return;
			}

			srv->module = srv->directory;

			HTREEITEM hItem=GetTreeCtrl().InsertItem(cvs::wide(srv->srvname.c_str()),1,1,GetTreeCtrl().GetRootItem());
			GetTreeCtrl().SetItemData(hItem,(DWORD_PTR)srv);
			GetTreeCtrl().InsertItem(_T("__@@__"),0,0,hItem);

			cvs::string key,rt;
			if(srv->root.size())
				rt = srv->root+"*"+srv->module;
			else
				cvs::sprintf(rt,80,"%s:%s",srv->server.c_str(),srv->port.c_str());
			cvs::sprintf(key,80,"Serv_%s",srv->srvname.c_str());
			CGlobalSettings::SetUserValue("WorkspaceManager","Servers",key.c_str(),rt.c_str());
		}
	}
}

void CWorkspaceViewerView::OnUpdateNew(CCmdUI *pCmdUI)
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(!dir)
			pCmdUI->Enable(TRUE);
		else
			pCmdUI->Enable(FALSE);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CWorkspaceViewerView::OnViewRevisions()
{
	m_bViewRevisions=!m_bViewRevisions;
	CGlobalSettings::SetUserValue("WorkspaceViewer",NULL,"ViewRevisions",m_bViewRevisions);
}

void CWorkspaceViewerView::OnUpdateViewRevisions(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewRevisions?1:0);
	pCmdUI->Enable();
}

void CWorkspaceViewerView::OnUpdateViewShowglobalservers(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bShowGlobal?1:0);
	pCmdUI->Enable();
}

void CWorkspaceViewerView::OnViewShowglobalservers()
{
	m_bShowGlobal=!m_bShowGlobal;
	CGlobalSettings::SetUserValue("WorkspaceViewer",NULL,"ViewRevisions",m_bViewRevisions);
	Refresh();
}



void CWorkspaceViewerView::OnUpdateMenuView(CCmdUI *pCmdUI)
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(dir && dir->isfile)
			pCmdUI->Enable(TRUE);
		else
			pCmdUI->Enable(FALSE);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CWorkspaceViewerView::OnMenuView()
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(dir && dir->level>0 && dir->isfile)
		{
			const char *cvsnt = CGlobalSettings::GetCvsCommand();

			CWaitCursor wait;

			CRunFile rf;
			rf.setError(_ViewFileErr,this);
			rf.setOutput(_ViewFileOut,this);
			rf.addArg(cvsnt);
			rf.addArg("-d");
			rf.addArg(dir->root.c_str());
			rf.addArg("co");
			rf.addArg("-p");
			if(dir->tag.size())
			{
				rf.addArg("-r");
				rf.addArg(dir->tag.c_str());
			}
			rf.addArg(dir->module.c_str());

			cvs::string fn = CFileAccess::tempfilename("cvs");
			m_viewFile.open(fn.c_str(),"w");
			if(!rf.run(NULL))
				AfxMessageBox(_T("Unable to run cvs"));
			else
			{
				int ret;
				rf.wait(ret);
			}
			m_viewFile.close();

			wait.Restore();

			CViewDlg dlg;
			dlg.m_szFile = fn;
			dlg.m_szTitle = dir->module;
			dlg.DoModal();		
		}
	}
}

int CWorkspaceViewerView::_ViewFileErr(const char *str, size_t len, void *param)
{
	return ((CWorkspaceViewerView*)param)->ViewFileErr(str,len);
}

int CWorkspaceViewerView::ViewFileErr(const char *str, size_t len)
{
	CServerIo::trace(3,"%-*.*s",str,len);
	return (int)len;
}

int CWorkspaceViewerView::_ViewFileOut(const char *str, size_t len, void *param)
{
	return ((CWorkspaceViewerView*)param)->ViewFileOut(str,len);
}

int CWorkspaceViewerView::ViewFileOut(const char *str, size_t len)
{
	m_viewFile.write(str,len);
	return (int)len;
}

void CWorkspaceViewerView::Error(ServerConnectionInfo *current, ServerConnectionError error)
{
	const TCHAR *message;
	switch(error)
	{
		case SCESuccessful:
			return;
		case SCEFailedNoAnonymous: message = _T("Logon failed an no anonymous user defined"); break;
		case SCEFailedBadExec: message = _T("Failed to execute cvs"); break;
		case SCEFailedConnection: message = _T("Connection to server failed"); break;
		case SCEFailedBadProtocol: message = _T("Connection to server failed due to bad protocol"); break;
		case SCEFailedNoSupport: message = _T("Server does not support remote repository listing"); break;
		case SCEFailedCommandAborted: message = _T("Command execution failed"); break;
		default: message = _T("Connection failed"); break;
	}
	AfxMessageBox(message, MB_ICONSTOP|MB_OK);
}

void CWorkspaceViewerView::OnUpdateMenuProperties(CCmdUI *pCmdUI)
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(dir && dir->level==0 && dir->user)
			pCmdUI->Enable(TRUE);
		else
			pCmdUI->Enable(FALSE);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CWorkspaceViewerView::OnMenuProperties()
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);

		if(dir && dir->user)
		{
			CBrowserRootDialog dlg(m_hWnd);
			if(dlg.ShowDialog(dir,"Properties",0))
			{
				cvs::string key,rt;
				if(dir->root.size())
					rt = dir->root+"*"+dir->module;
				else
					cvs::sprintf(rt,80,"%s:%s",dir->server.c_str(),dir->port.c_str());
				cvs::sprintf(key,80,"Serv_%s",dir->srvname.c_str());
				CGlobalSettings::SetUserValue("WorkspaceManager","Servers",key.c_str(),rt.c_str());
			}
		}
	}
}

void CWorkspaceViewerView::OnUpdateMenuDelete(CCmdUI *pCmdUI)
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(dir && dir->level==0 && dir->user)
			pCmdUI->Enable(TRUE);
		else
			pCmdUI->Enable(FALSE);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CWorkspaceViewerView::OnMenuDelete()
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if(hItem)
	{
		ServerConnectionInfo *dir = (ServerConnectionInfo *)GetTreeCtrl().GetItemData(hItem);
		if(dir && dir->level==0 && dir->user)
		{
			cvs::string key;
			CString str = GetTreeCtrl().GetItemText(hItem);
			cvs::sprintf(key,80,"Serv_%s",(const char *)cvs::narrow(str));
			CGlobalSettings::DeleteUserValue("WorkspaceManager","Servers",key.c_str());
			GetTreeCtrl().DeleteItem(hItem);
		}
	}
}
