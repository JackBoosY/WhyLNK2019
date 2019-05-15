
// WhyLNK2019.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CWhyLNK2019App:
// See WhyLNK2019.cpp for the implementation of this class
//

class CWhyLNK2019App : public CWinApp
{
public:
	CWhyLNK2019App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CWhyLNK2019App theApp;
