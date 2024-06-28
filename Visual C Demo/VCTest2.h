// VCTest2.h : main header file for the VCTEST2 application
//

#if !defined(AFX_VCTEST2_H__9387F416_A99A_11D1_849A_00A0C9089C5A__INCLUDED_)
#define AFX_VCTEST2_H__9387F416_A99A_11D1_849A_00A0C9089C5A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVCTest2App:
// See VCTest2.cpp for the implementation of this class
//

class CVCTest2App : public CWinApp
{
public:
	CVCTest2App();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVCTest2App)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVCTest2App)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCTEST2_H__9387F416_A99A_11D1_849A_00A0C9089C5A__INCLUDED_)
