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
// WorkspaceViewerView.h : interface of the CWorkspaceViewerView class
//


#pragma once


class CWorkspaceViewerView : public CTreeView, public CServerConnectionCallback
{
protected: // create from serialization only
	CWorkspaceViewerView();
	DECLARE_DYNCREATE(CWorkspaceViewerView)

public:
	CWorkspaceViewerDoc* GetDocument() const;

protected:
	std::vector<cvs::string> m_dirList;
	std::map<cvs::string,int> m_tagList;
	typedef std::map<std::string, std::string> servers_t;
	static servers_t m_servers;
	int m_error;
	CFileAccess m_viewFile;

	bool m_bViewFiles;
	bool m_bViewTags;
	bool m_bViewBranches;
	bool m_bViewRevisions;
	bool m_bShowGlobal;

	ServerConnectionInfo *m_menuObj;

	virtual ~CWorkspaceViewerView();

	static int _BuildTagList(const char *data,size_t len,void *param);
	static int _ViewFileOut(const char *str, size_t len, void *param);
	int ViewFileOut(const char *str, size_t len);
	static int _ViewFileErr(const char *str, size_t len, void *param);
	int ViewFileErr(const char *str, size_t len);
	int BuildTagList(const char *data,size_t len);
	void Refresh();

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();

	// CServerConnectionCallback
	virtual bool AskForPassword(ServerConnectionInfo *dir);
	virtual void ProcessOutput(const char *line);
	virtual void Error(ServerConnectionInfo *current, ServerConnectionError error);

	afx_msg void OnTvnDeleteitem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnViewFiles();
	afx_msg void OnUpdateViewFiles(CCmdUI *pCmdUI);
	afx_msg void OnViewTags();
	afx_msg void OnUpdateViewTags(CCmdUI *pCmdUI);
	afx_msg void OnViewBranches();
	afx_msg void OnUpdateViewBranches(CCmdUI *pCmdUI);
	afx_msg void OnNew();
	afx_msg void OnUpdateNew(CCmdUI *pCmdUI);
	afx_msg void OnViewRevisions();
	afx_msg void OnUpdateViewRevisions(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewShowglobalservers(CCmdUI *pCmdUI);
	afx_msg void OnViewShowglobalservers();
	afx_msg void OnUpdateMenuView(CCmdUI *pCmdUI);
	afx_msg void OnMenuView();
	afx_msg void OnUpdateMenuProperties(CCmdUI *pCmdUI);
	afx_msg void OnMenuProperties();
	afx_msg void OnUpdateMenuDelete(CCmdUI *pCmdUI);
	afx_msg void OnMenuDelete();
};

inline CWorkspaceViewerDoc* CWorkspaceViewerView::GetDocument() const
   { return reinterpret_cast<CWorkspaceViewerDoc*>(m_pDocument); }

