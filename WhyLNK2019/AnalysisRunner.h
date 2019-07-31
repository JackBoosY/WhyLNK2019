#pragma once

#include <string>
#include <map>

typedef enum _en_arch
{
	EN_X86,
	EN_X64
}en_arch;

class AnalysisRunner
{
public:
	AnalysisRunner();
	~AnalysisRunner();

	static en_arch GetSysArch();

public:
	bool BeginAnaylsis(std::string strErrors, en_arch tarArch, std::string strLibName, std::string strLibPath, std::string& strResult);

private:
	bool AnaylsisErrors(std::string strErrors, std::map<std::string, std::string>& functions);

private:
	en_arch m_arch;
};

