
// WhyLNK2019Dlg.h : header file
//

#pragma once
#include "AnalysisRunner.h"


// CWhyLNK2019Dlg dialog
class CWhyLNK2019Dlg : public CDialogEx
{
// Construction
public:
	CWhyLNK2019Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WHYLNK2019_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	BOOL CheckVars();

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	// Options of Archtect
	CComboBox m_cbArch;
	afx_msg void OnBnClickedButtonRun();
	// Errors whose we met// Errors whose we met
	CEdit m_editErrors;
	// Used library name
	CEdit m_editLibname;
	// Used library path
	CEdit m_editLibpath;
	// Show the result here
	CEdit m_editResult;
private:
	en_arch m_arch;
	CString m_strLibname;
	CString m_strLibpath;
	CString m_strErrors;
	en_arch m_tarArch;
	AnalysisRunner m_runner;
};
