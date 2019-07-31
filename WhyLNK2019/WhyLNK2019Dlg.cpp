
// WhyLNK2019Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "WhyLNK2019.h"
#include "WhyLNK2019Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWhyLNK2019Dlg dialog

CWhyLNK2019Dlg::CWhyLNK2019Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WHYLNK2019_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWhyLNK2019Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_ARCH, m_cbArch);
	//  DDX_Control(pDX, IDC_EDIT_ERRORS, m_strErrors);
	//  DDX_Control(pDX, IDC_EDIT_LIB_NAME, m_strLibName);
	DDX_Control(pDX, IDC_EDIT_ERRORS, m_editErrors);
	DDX_Control(pDX, IDC_EDIT_LIB_NAME, m_editLibname);
	DDX_Control(pDX, IDC_EDIT_LIB_PATH, m_editLibpath);
	DDX_Control(pDX, IDC_EDIT_RESULT, m_editResult);
}

BOOL CWhyLNK2019Dlg::CheckVars()
{
	CString strVar;
	m_editLibname.GetWindowText(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_strLibname = strVar;

	m_editLibpath.GetWindowText(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_strLibpath = strVar;

	m_editErrors.GetWindowText(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_strErrors = strVar;

	m_cbArch.GetWindowText(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_tarArch = strVar == "x86" ? EN_X86 : EN_X64;

	return TRUE;
}

BEGIN_MESSAGE_MAP(CWhyLNK2019Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CWhyLNK2019Dlg::OnBnClickedButtonRun)
END_MESSAGE_MAP()


// CWhyLNK2019Dlg message handlers

BOOL CWhyLNK2019Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_cbArch.SetCurSel(0);
	// Check this machine's arch
	m_arch = m_runner.GetSysArch();

	//test
	m_editLibname.SetWindowText("fmt.dll");
	m_editLibpath.SetWindowText("F:\\vcpkg\\installed\\x86-windows\\bin");
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWhyLNK2019Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWhyLNK2019Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWhyLNK2019Dlg::OnBnClickedButtonRun()
{
	// Check vars
	if (FALSE == CheckVars())
	{
		m_editResult.SetWindowText("parameters needs correct.");
		return;
	}

	// Analysis errors
	std::string strOutResult;
	if (FALSE == m_runner.BeginAnaylsis((LPCTSTR)m_strErrors, m_tarArch, (LPCTSTR)m_strLibname, (LPCTSTR)m_strLibpath, strOutResult))
	{
		m_editResult.SetWindowText(strOutResult.c_str());
		return;
	}

	// Output results
	m_editResult.SetWindowText(strOutResult.c_str());
}
