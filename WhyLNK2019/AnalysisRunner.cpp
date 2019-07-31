#include "AnalysisRunner.h"
#include <vector>
#include <windows.h>
#include <process.h>
#include <atlconv.h>
#include <sstream>
#include <fstream>
#include <json/json.h>

AnalysisRunner::AnalysisRunner()
{
	m_arch = GetSysArch();
}


AnalysisRunner::~AnalysisRunner()
{
}

en_arch AnalysisRunner::GetSysArch()
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

bool AnalysisRunner::BeginAnaylsis(std::string strErrors, en_arch tarArch, std::string strLibName, std::string strLibPath, std::string& strResult)
{
	// Analysis errors
	std::map<std::string, std::string> errorMap;
	if (FALSE == AnaylsisErrors(strErrors, errorMap))
	{
		strResult = "analysis failed.";
		return false;
	}

	// Check related functions
	std::string strExeName = "TryToLink_";
	switch (tarArch)
	{
	case EN_X86:
		strExeName += "x86.exe";
		break;
	case EN_X64:
		strExeName += "x64.exe";
		break;
	default:
		strResult = "analysis failed.";
		return false;
		break;
	}

	std::string strFunctions;
	for (std::map<std::string, std::string>::iterator i = errorMap.begin(); i != errorMap.end(); i++)
	{
		strFunctions += i->first;
		strFunctions += ";";
	}

	// Launch analysis exe
	STARTUPINFO si = { sizeof(si) };
	memset(&si, 0, sizeof(STARTUPINFO));
	PROCESS_INFORMATION info;
	memset(&info, 0, sizeof(PROCESS_INFORMATION));
	if (!::CreateProcess(strExeName.c_str(), NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &info))
	{
		strResult = "analysis failed.";
		int i = GetLastError();
		return false;
	}

	// Connect to pipe
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	for (auto i = 0; i < 3; i++)
	{
		hPipe = CreateFile("\\\\.\\pipe\\WhyLNK2019", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

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
		strResult = "analysis failed: Create pipe failed.";
		return false;
	}

	// Send params to client
	Json::Value root;
	root["srcArch"] = m_arch;
	root["dstArch"] = tarArch;
	root["libName"] = strLibName;
	root["libPath"] = strLibPath;
	root["Functions"] = strFunctions;
	std::ostringstream ss;
	Json::StreamWriterBuilder writer;
	std::unique_ptr<Json::StreamWriter> write(writer.newStreamWriter());
	write->write(root, &ss);

	DWORD dwWritten = 0;
	BOOL bRet = WriteFile(hPipe, ss.str().c_str(), ss.str().length() + 1, &dwWritten, NULL);
	if (!bRet || dwWritten == 0)
	{
		strResult = "analysis failed: Send params failed.";
		return false;
	}

	// Read results from pipe
	wchar_t* wCmd = NULL;
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
				strResult = "analysis failed: Read result failed.";
				return false;
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
		strResult = "analysis failed: parse result error.";
		return false;
	}

	int iError = rsltRoot["Ret"].asInt();
	if (0 == iError)
	{
		strResult = "analysis success:\r\n";
		Json::Value::Members mem = rsltRoot["Result"].getMemberNames();
		for (auto i = mem.begin(); i != mem.end(); i++)
		{
			strResult += i->c_str();
			strResult += ": ";
			strResult += (rsltRoot["Result"][*i].asBool() ? "Success\r\n" : "Failed\r\n");
		}
		return true;
	}
	else
	{
		strResult = "analysis failed.";
		return false;
	}
}

bool AnalysisRunner::AnaylsisErrors(std::string strErrors, std::map<std::string, std::string>& functions)
{
	std::vector<std::string> strLines;
	std::string sep;
	if (strErrors.find("\r", 0) != -1)
	{
		// Win style
		sep = "\r\n";
	}
	else
	{
		// Uinx Style
		sep = "\n";
	}

	int nPos = strErrors.find(sep);
	std::string strLeft = "";

	while (0 <= nPos)
	{
		strLeft = strErrors[nPos];
		if (!strLeft.empty())
			strLines.push_back(strLeft);
		strErrors = strErrors.substr(strErrors.length() - nPos - sep.length());
		nPos = strErrors.find(sep);
	}

	if (!strErrors.empty())
	{
		strLines.push_back(strErrors);
	}

	// Delete line which is not lnk error
	std::vector<std::string> tmpVec;
	for (int i = 0; i < strLines.size(); i++)
	{
		if (strLines[i].find("LNK") != -1 //Not lnk error
			&& strLines[i].find("unresolved externals") == -1) //Final result
		{
			tmpVec.push_back(strLines[i]);
		}
	}
	strLines.swap(tmpVec);

	if (strLines.empty())
	{
		return false;
	}

	//Analysis each error line
	for (auto i = 0; i < strLines.size(); i++)
	{
		std::string strErr = strLines[i];
		std::string strFuncName;
		int iBegin = strErr.find("unresolved external symbol ");
		iBegin += std::string("unresolved external symbol ").length();
		int iEnd = strErr.find(" ", iBegin);
		strFuncName = strErr.substr(iBegin, iEnd - iBegin);
		functions[strFuncName] = "";

	}

	return true;
}