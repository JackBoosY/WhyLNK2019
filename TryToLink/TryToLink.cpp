// TryToLink.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <windows.h>
#include <json/json.h>

typedef PVOID(CALLBACK* PFNEXPORTFUNC) (PIMAGE_NT_HEADERS, PVOID, ULONG, PIMAGE_SECTION_HEADER*);

// Get All Symbols in the library.
bool GetAllSymbols(const char* fileName, std::vector<std::string>& symbolList)
{
	LPWIN32_FIND_DATA lpwfd_first = new WIN32_FIND_DATA;
	HANDLE hFile, hFileMap;
	DWORD fileAttrib = 0;
	void* mode_base = nullptr;

	PFNEXPORTFUNC ImageRvaToVax = nullptr;
	HMODULE hModule = ::LoadLibrary("DbgHelp.dll");
	if (hModule)
	{
		ImageRvaToVax = (PFNEXPORTFUNC)::GetProcAddress(hModule, "ImageRvaToVa");
		if (!ImageRvaToVax)
		{
			::FreeLibrary(hModule);
			return false;
		}
	}
	else
	{
		return false;
	}

	if (FindFirstFile(fileName, lpwfd_first) == nullptr)
	{
		return false;
	}

	fileAttrib = lpwfd_first->dwFileAttributes;
	hFile = CreateFile(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, fileAttrib, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	hFileMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if (!hFileMap)
	{
		CloseHandle(hFile);
		return false;
	}

	mode_base = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
	if (!mode_base)
	{
		CloseHandle(hFileMap);
		CloseHandle(hFile);
		return false;
	}

	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)mode_base;
	IMAGE_NT_HEADERS* pNTHeader = (IMAGE_NT_HEADERS*)((BYTE*)mode_base + pDosHeader->e_lfanew);
	IMAGE_OPTIONAL_HEADER* pOptHeader = (IMAGE_OPTIONAL_HEADER*)((BYTE*)mode_base + pDosHeader->e_lfanew + 24);
	IMAGE_EXPORT_DIRECTORY* pExportDesc = (IMAGE_EXPORT_DIRECTORY*)ImageRvaToVax(pNTHeader, mode_base, pOptHeader->DataDirectory[0].VirtualAddress, 0);

	PDWORD nameAddr = (PDWORD)ImageRvaToVax(pNTHeader, mode_base, pExportDesc->AddressOfNames, 0);
	PCHAR func_name = (PCHAR)ImageRvaToVax(pNTHeader, mode_base, (DWORD)nameAddr[0], 0);
	symbolList.push_back(func_name);

	DWORD unti = pExportDesc->NumberOfNames;
	
	for (DWORD i = 0; i < unti; i++)
	{
		func_name = (PCHAR)ImageRvaToVax(pNTHeader, mode_base, (DWORD)nameAddr[i], 0);
		symbolList.push_back(func_name);
	}

	::FreeLibrary(hModule);
	CloseHandle(hFileMap);
	CloseHandle(hFile);

	return true;
}

// Test symbols whose link failure in library.
int TryToLinkFuctions(const char* fileName, const char* functions, std::map<std::string, bool>& resultMap)
{
	std::map<std::string, bool> realNameMap;

	// Handle the function names.
	const char* pName = functions;
	std::string fullName;
	while (*pName != '\0')
	{
		if (*pName == ';')
		{
			realNameMap.insert(std::pair<std::string, bool>(fullName, false));
			fullName.clear();
		}
		else
		{
			fullName.append(1, (char)*pName);
		}
		pName++;
	}

	HMODULE hHandle = ::LoadLibrary(fileName);
	if (hHandle)
	{
		// Start load functions.
		for (auto i = realNameMap.begin(); i != realNameMap.end(); i++)
		{
			i->second = (::GetProcAddress(hHandle, i->first.c_str()) != nullptr);
		}

		::FreeLibrary(hHandle);
	}
	else
	{
		int iError = GetLastError();
		printf("Load library failed: %d\nMissing dependence library.", iError);
		return iError;
	}

	resultMap.swap(realNameMap);

	return 0;
}

int main(int argc, char** argv)
{
	// Connect to pipe
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	hPipe = CreateNamedPipe("\\\\.\\pipe\\WhyLNK2019", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE
		| PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 2048, 2048, 0, NULL);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		printf("Create Pipe failed: %d", GetLastError());
		return -1;
	}
	// Wait for connect
	printf("Waiting for connect...\n");
	ConnectNamedPipe(hPipe, NULL);

	// Read params
	std::string strParams;
	char szBufRecv[1024] = { 0 };
	DWORD dwReadSize = 0;
	while (true)
	{
		memset(szBufRecv, 1024, 0);
		BOOL bRet = ::ReadFile(hPipe, szBufRecv, 1024, &dwReadSize, NULL);
		if (!bRet)
		{
			DWORD dwLastError = ::GetLastError();
			if (dwLastError == ERROR_PIPE_LISTENING)
				continue;
			else
			{
				break;
			}
		}
		else if (dwReadSize == 0 || dwReadSize < 1024)
		{
			strParams += szBufRecv;
			break;
		}
		else
		{
			printf("Reciving params...\n");
			strParams += szBufRecv;
		}
	}
	Json::Value vRoot;
	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;

	std::istringstream ifs(strParams);
	std::string strError;
	printf("Parsing params...\n");
	if (!parseFromStream(builder, ifs, &vRoot, &strError))
	{
		printf("parse params failed.");
		::CloseHandle(hPipe);
		return 0;
	}

	// Get Library Symbols.
	std::string strLibFullPath = vRoot["libPath"].asCString();
	strLibFullPath += "\\";
	strLibFullPath += vRoot["libName"].asCString();

	std::vector<std::string> symbolList;
	printf("Get library symbols...\n");
	if (!GetAllSymbols(strLibFullPath.c_str(), symbolList))
	{
		printf("Get symbols failed.");
		return -1;
	}

	// Retry link to the library and collect results.
	std::map<std::string, bool> resultMap;
	printf("Retry library symbols...\n");
	int iRes = TryToLinkFuctions(strLibFullPath.c_str(), vRoot["Functions"].asCString(), resultMap);

	// Generate results to json file.
	Json::Value root, result;
	for (auto i = resultMap.begin(); i != resultMap.end(); i++)
	{
		result[i->first] = i->second;
	}
	root["Ret"] = iRes;
	root["Result"] = result;

	std::ostringstream ss;
	Json::StreamWriterBuilder writer;
	std::unique_ptr<Json::StreamWriter> write(writer.newStreamWriter());
	write->write(root, &ss);

	// Transform to caller
	// Send result
	DWORD dwWritten = 0;
	BOOL bRet = WriteFile(hPipe, ss.str().c_str(), ss.str().length() + 1, &dwWritten, NULL);
	if (!bRet || dwWritten == 0)
	{
		printf("WriteFile failed: %d", GetLastError());
		return -1;
	}
	while (true)
	{
		char szBufRecv[1024] = { 0 };
		DWORD dwReadSize = 0;
		BOOL bRet = ::ReadFile(hPipe, szBufRecv, 1024, &dwReadSize, NULL);
		if (!bRet || dwReadSize == 0)
		{
			DWORD dwLastError = ::GetLastError();
			if (dwLastError == ERROR_PIPE_LISTENING)
				continue;
			else
				break;
		}
	}

	::CloseHandle(hPipe);

	printf("Done.\n");
	return 0;
}