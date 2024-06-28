// VCTest2Dlg.h : header file
//

#if !defined(AFX_VCTEST2DLG_H__9387F418_A99A_11D1_849A_00A0C9089C5A__INCLUDED_)
#define AFX_VCTEST2DLG_H__9387F418_A99A_11D1_849A_00A0C9089C5A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CVCTest2Dlg dialog

class CVCTest2Dlg : public CDialog
{
// Construction
public:
	CVCTest2Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVCTest2Dlg)
	enum { IDD = IDD_VCTEST2_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVCTest2Dlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CVCTest2Dlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCTEST2DLG_H__9387F418_A99A_11D1_849A_00A0C9089C5A__INCLUDED_)
