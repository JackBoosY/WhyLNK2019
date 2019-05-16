// TryToLink.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <map>

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
void TryToLinkFuctions(const char* fileName, const char* functions, std::map<std::string, bool>& resultMap)
{
	std::map<std::string, bool> realNameMap, fakeNameMap;

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
	
	for (auto i = realNameMap.begin(); i != realNameMap.end(); i++)
	{
		fakeNameMap.insert(std::pair<std::string, bool>(i->first, false));
	}

	HMODULE hHandle = ::LoadLibrary(fileName);
	if (hHandle)
	{
		// Start load functions.
		for (auto i = fakeNameMap.begin(); i != fakeNameMap.end(); i++)
		{
			i->second = (::GetProcAddress(hHandle, i->first.c_str()) != nullptr);
		}
		// Copy result.
		for (auto i = realNameMap.begin(), j = fakeNameMap.begin(); i != realNameMap.end(), j != fakeNameMap.end(); i++, j++)
		{
			i->second = j->second;
		}

		::FreeLibrary(hHandle);
	}

	resultMap.swap(realNameMap);

	return;
}

int main(int argc, char** argv)
{
	const char* pszLibName = argv[1];
	const char* pszLibPath = argv[2];
	const char* pszFucntionsName = argv[3];

	std::string strLibFullPath = pszLibPath;
	strLibFullPath += "\\";
	strLibFullPath += pszLibName;

	std::vector<std::string> symbolList;
	// Get Library Symbols.
	if (!GetAllSymbols(strLibFullPath.c_str(), symbolList))
	{
		return -1;
	}

	// Retry link to the library and collect results.
	std::map<std::string, bool> resultMap;
	TryToLinkFuctions(strLibFullPath.c_str(), pszFucntionsName, resultMap);

	// Generate results to json file.

	return 0;
}