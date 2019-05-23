
// WhyLNK2019Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "WhyLNK2019.h"
#include "WhyLNK2019Dlg.h"
#include "afxdialogex.h"

#include <process.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <windows.h>
#include <atlconv.h>
#include <json/json.h>

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
	m_editLibname.GetWindowTextW(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_strLibname = strVar;

	m_editLibpath.GetWindowTextW(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_strLibpath = strVar;

	m_editErrors.GetWindowTextW(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_strErrors = strVar;

	m_cbArch.GetWindowTextW(strVar);
	if (strVar.IsEmpty())
	{
		return FALSE;
	}
	m_tarArch = strVar == L"x86" ? EN_X86 : EN_X64;

	return TRUE;
}

en_arch CWhyLNK2019Dlg::GetSysArch()
{
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_IA64)
	{
		return EN_X64;
	}
	else
	{
		return EN_X86;
	}
}

BOOL CWhyLNK2019Dlg::AnaylsisErrors(std::map<CString, CString>& functions)
{
	CString strErrors = m_strErrors;

	std::vector<CString> strLines;
	CString sep;
	if (strErrors.Find(L"\r", 0) != -1)
	{
		// Win style
		sep = L"\r\n";
	}
	else
	{
		// Uinx Style
		sep = L"\n";
	}

	int nPos = strErrors.Find(sep);
	CString strLeft = L"";

	while (0 <= nPos)
	{
		strLeft = strErrors.Left(nPos);
		if (!strLeft.IsEmpty())
			strLines.push_back(strLeft);
		strErrors = strErrors.Right(strErrors.GetLength() - nPos - sep.GetLength());
		nPos = strErrors.Find(sep);
	}

	if (!strErrors.IsEmpty())
	{
		strLines.push_back(strErrors);
	}

	// Delete line which is not lnk error
	std::vector<CString> tmpVec;
	for (int i = 0; i < strLines.size(); i++)
	{
		if (strLines[i].Find(L"LNK") != -1 //Not lnk error
			&& strLines[i].Find(L"unresolved externals") == -1) //Final result
		{
			tmpVec.push_back(strLines[i]);
		}
	}
	strLines.swap(tmpVec);

	if (strLines.empty())
	{
		return FALSE;
	}

	//Analysis each error line
	for (auto i = 0; i < strLines.size(); i++)
	{
		CString strErr = strLines[i];
		CString strFuncName;
		int iBegin = strErr.Find(L"unresolved external symbol ");
		iBegin += CString(L"unresolved external symbol ").GetLength();
		int iEnd = strErr.Find(L" ", iBegin);
		strFuncName = strErr.Mid(iBegin, iEnd - iBegin);
		functions[strFuncName] = "";

	}

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
	m_arch = GetSysArch();

	//test
	m_editLibname.SetWindowTextW(L"fmt.dll");
	m_editLibpath.SetWindowTextW(L"F:\\vcpkg\\installed\\x86-windows\\bin");
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
		m_editResult.SetWindowTextW(L"parameters needs correct.");
		return;
	}

	// Analysis errors
	std::map<CString, CString> m_errorMap;
	if (FALSE == AnaylsisErrors(m_errorMap))
	{
		m_editResult.SetWindowTextW(L"analysis failed.");
		return;
	}

	// Check related functions
	CString strExeName = L"TryToLink_";
	switch (m_tarArch)
	{
	case EN_X86:
		strExeName += L"x86.exe";
		break;
	case EN_X64:
		strExeName += L"x64.exe";
		break;
	default:
		return;
		break;
	}

	CString strFunctions;
	for (std::map<CString, CString>::iterator i = m_errorMap.begin(); i != m_errorMap.end(); i++)
	{
		strFunctions += i->first;
		strFunctions += ";";
	}

	// Launch analysis exe
	STARTUPINFO si = { sizeof(si) };
	memset(&si, 0, sizeof(STARTUPINFO));
	PROCESS_INFORMATION info;
	memset(&info, 0, sizeof(PROCESS_INFORMATION));
	if (!::CreateProcessW(strExeName, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &info))
	{
		int i = GetLastError();
		return;
	}

	// Connect to pipe
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	for (auto i = 0; i < 3; i++)
	{
		hPipe = CreateFile(L"\\\\.\\pipe\\WhyLNK2019", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			// Wait for client open pipe.
			Sleep(200);
			continue;
		}
		else
		{
			break;
		}
	}

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		int iError = GetLastError();
		m_editResult.SetWindowTextW(L"analysis failed: Create pipe failed.");
		return;
	}

	// Send params to client
	Json::Value root;
	root["srcArch"] = m_arch;
	root["dstArch"] = m_tarArch;
	root["libName"] = (const char*)_bstr_t((LPCTSTR)m_strLibname);
	root["libPath"] = (const char*)_bstr_t((LPCTSTR)m_strLibpath);
	root["Functions"] = (const char*)_bstr_t((LPCTSTR)strFunctions);
	std::ostringstream ss;
	Json::StreamWriterBuilder writer;
	std::unique_ptr<Json::StreamWriter> write(writer.newStreamWriter());
	write->write(root, &ss);

	DWORD dwWritten = 0;
	BOOL bRet = WriteFile(hPipe, ss.str().c_str(), ss.str().length() + 1, &dwWritten, NULL);
	if (!bRet || dwWritten == 0)
	{
		m_editResult.SetWindowTextW(L"analysis failed: Send params failed.");
		return;
	}

	// Read results from pipe
	wchar_t* wCmd = NULL;
	std::string strResult;
	char szBufRecv[1024] = { 0 };
	DWORD dwReadSize = 0;
	while (true)
	{
		BOOL bRet = ::ReadFile(hPipe, szBufRecv, 1023, &dwReadSize, NULL);
		if (!bRet)
		{
			DWORD dwLastError = ::GetLastError();
			if (dwLastError == ERROR_PIPE_LISTENING)
				continue;
			else
			{
				DWORD dwWriteSize = 0;
				::WriteFile(hPipe, "done", strlen("done") + 1, &dwWriteSize, NULL);
				::CloseHandle(hPipe);
				m_editResult.SetWindowTextW(L"analysis failed: Read result failed.");
				return;
			}
		}
		else if (dwReadSize == 0 || dwReadSize < 1023)
		{
			DWORD dwWriteSize = 0;
			::WriteFile(hPipe, "done", strlen("done") + 1, &dwWriteSize, NULL);
			::CloseHandle(hPipe);
			strResult += szBufRecv;
			break;
		}
		else
		{
			DWORD dwWriteSize = 0;
			::WriteFile(hPipe, "done", strlen("done") + 1, &dwWriteSize, NULL);
			strResult += szBufRecv;
		}
	}

	// Parse return results.
	Json::Value rsltRoot;
	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	std::istringstream ifs(strResult);
	std::string strError;
	printf("Parsing params...\n");
	if (!parseFromStream(builder, ifs, &rsltRoot, &strError))
	{
		m_editResult.SetWindowTextW(L"analysis failed: parse result error.");
		return;
	}

	CString strOut;
	int iError = rsltRoot["Ret"].asInt();
	if (0 == iError)
	{
		strOut = L"analysis success:\r\n";
		Json::Value::Members mem = rsltRoot["Result"].getMemberNames();
		for (auto i = mem.begin(); i != mem.end(); i++)
		{
			const size_t cSize = strlen(i->c_str()) + 1;
			wchar_t* wc = new wchar_t[cSize];
			mbstowcs(wc, i->c_str(), cSize);
			strOut += wc;
			delete[] wc;

			strOut += L": ";

			strOut += (rsltRoot["Result"][*i].asBool() ? L"Success\r\n" : L"Failed\r\n");
		}

	}
	else
	{
		strOut = L"analysis failed.";
	}

	// Output results
	m_editResult.SetWindowTextW(strOut);
}
