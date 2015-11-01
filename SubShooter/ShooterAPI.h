#pragma once
#include "curl.h"
#include "json.h"

typedef struct {
	char *memory;
	size_t size;
}MEMORY_CHUNK;

class CShooterAPI
{
private:
	char	m_szError[1024];
	bool	m_bError;
	unsigned char ToHex(unsigned char x) { return  x > 9 ? x + 55 : x + 48; };
	void	urlEncode(char* encodedString, int nStrLen, const char* str);

private:
	CURL*	m_curl;
	MEMORY_CHUNK m_chunk;
	bool	GetVideoHashParam(char* szParam, int nParamLen, const char* filename, bool langChn = true);
	bool	RequestSubJSON(const char* szParams);
	bool	GetFirstLink(char* szLink, char* szExt);
	bool	DownloadSubFile(const char* szFilename, const char* szURL);

	static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

public:
	CShooterAPI();
	~CShooterAPI();
	bool GetFirstSubTitle(const char* szVideoFilename, const char* szDefSubFilename = NULL, bool langChn = true);
	char* GetError() { return m_szError; };
};

