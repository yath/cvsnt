#pragma once


// CViewDlg dialog

class CViewDlg : public CDialog
{
	DECLARE_DYNAMIC(CViewDlg)

public:
	cvs::string m_szFile, m_szTitle;

	CViewDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CViewDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG3 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CEdit m_edText;
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
