#include "stdafx.h"
#include "ShooterAPI.h"
#include "MD5.h"


CShooterAPI::CShooterAPI(){
	m_curl = NULL;
	m_bError = false;
	strcpy_s(m_szError, 1024, "");
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res != CURLE_OK) {
		strcpy_s(m_szError, 256, "\nError: Unable to init socket.\n");
		m_bError = true;
	}
	m_curl = curl_easy_init();
	if (m_curl == NULL) {
		strcpy_s(m_szError, 256, "\nError: Unable to init socket.\n");
		m_bError = true;
	}
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)&m_chunk);

}


CShooterAPI::~CShooterAPI(){
	if (m_curl != NULL) {
		curl_easy_cleanup(m_curl);
	}
	curl_global_cleanup();
}

bool CShooterAPI::GetFirstSubTitle(const char* szVideoFilename, const char* szDefgSubFilename, bool landChn) {
	
	if (m_bError) {
		return false;
	}

	char szParams[1024];
	
	if (GetVideoHashParam(szParams, 1024, szVideoFilename, landChn) == false) {
		return false;
	}
	
	if (RequestSubJSON(szParams) == false) {
		return false;
	}


	char szLink[1024];
	char szExt[16];
	if (GetFirstLink(szLink, szExt) == false) {
		free(m_chunk.memory);
		strcpy_s(m_szError, 256, "\nError: Unable to find sub title for this video.\n");
		return false;
	}

	char* szSubFilename = NULL;
	int l = 0;
	if (szDefgSubFilename == NULL) {
		l = strlen(szVideoFilename) + 1;
		szSubFilename = new char[l];
		strcpy_s(szSubFilename, l, szVideoFilename);
		for (int i = l - 2; i >= 0; i--) {
			if (szSubFilename[i] == '.') {
				break;
			}
			szSubFilename[i] = 0;
		}
		strcat_s(szSubFilename, l, szExt);
	}
	else {
		l = strlen(szDefgSubFilename) + 1;
		strcpy_s(szSubFilename, l, szDefgSubFilename);
	}

	if (DownloadSubFile(szSubFilename, szLink) == false) {
		delete szSubFilename;
		return false;
	}
	delete szSubFilename;
	return true;
}

bool CShooterAPI::GetVideoHashParam(char* szParam, int nParamLen, const char* filename, bool langChn) {
	//Generate hash for video and build POST params
	int stream = 0;
	errno_t err = _sopen_s(&stream, filename, _O_BINARY | _O_RDONLY, _SH_DENYNO, _S_IREAD);
	if (err != 0) {
		sprintf_s(m_szError, 1024, "\nError: Unable to open video file <%s>.\n", filename);
		return false;
	}
	__int64 size = _filelengthi64(stream);
	if (size < 8192) {
		strcpy_s(m_szError, 1024, "\nError: This video is too short.\n");
		_close(stream);
		return false;
	}

	unsigned char buf[4096];
	char szHash[256];
	char szMD5[64];
	//part 1
	_lseeki64(stream, 4096, 0);
	_read(stream, buf, 4096);

	MD5 md5;
	md5.Generate(szHash, buf, 4096);

	//part 2
	_lseeki64(stream, size / 3 * 2, 0);
	_read(stream, buf, 4096);
	md5.Generate(szMD5, buf, 4096);
	strcat_s(szHash, 256, ";");
	strcat_s(szHash, 256, szMD5);

	//part 3
	_lseeki64(stream, size / 3, 0);
	_read(stream, buf, 4096);
	md5.Generate(szMD5, buf, 4096);
	strcat_s(szHash, 256, ";");
	strcat_s(szHash, 256, szMD5);

	//part 4
	_lseeki64(stream, size - 8192, 0);
	_read(stream, buf, 4096);
	md5.Generate(szMD5, buf, 4096);
	strcat_s(szHash, 256, ";");
	strcat_s(szHash, 256, szMD5);

	_close(stream);

	int l = strlen(filename) * 3;
	char* encodedFilename = new char[l + 1];
	urlEncode(encodedFilename, l, filename);
	l = strlen(szHash) * 3;
	char* encodedHash = new char[l + 1];
	urlEncode(encodedHash, l, szHash);

	sprintf_s(szParam, nParamLen, "filehash=%s&pathinfo=%s&format=json&lang=%s", encodedHash, encodedFilename, langChn ? "Chn" : "Eng");
	delete encodedFilename;
	delete encodedHash;
	return true;
}


void CShooterAPI::urlEncode(char* encodedString, int nStrLen, const char* str) {
	int l = strlen(str);
	strcpy_s(encodedString, nStrLen, "");
	char buf[4];
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	for (int i = 0; i < l; i++) {
		if (isalnum((unsigned char)str[i]) || (str[i] == '-') || (str[i] == '_') || (str[i] == '.') || (str[i] == '~')) {
			buf[0] = str[i];
			buf[1] = 0;
			buf[2] = 0;
		}
		else if (str[i] == ' ') {
			buf[0] = '+';
			buf[1] = 0;
			buf[2] = 0;
		}
		else {
			buf[0] = '%';
			buf[1] = ToHex((unsigned char)str[i] >> 4);
			buf[2] = ToHex((unsigned char)str[i] % 16);
			buf[3] = 0;
		}
		strcat_s(encodedString, nStrLen, buf);
	}
}

bool CShooterAPI::RequestSubJSON(const char* szParams) {
	curl_easy_setopt(m_curl, CURLOPT_URL, "https://www.shooter.cn/api/subapi.php");
	curl_easy_setopt(m_curl, CURLOPT_POST, 1);
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, szParams);
	m_chunk.memory = (char*)malloc(1);
	m_chunk.size = 0;
	if (curl_easy_perform(m_curl) != CURLE_OK) {
		if (m_chunk.size > 0) {
			free(m_chunk.memory);
		}
		strcpy_s(m_szError, 256, "\nError: Get sub failed.\n");
		return false;
	}
	return true;
}


bool CShooterAPI::GetFirstLink(char* szLink, char* szExt) {
	if (m_chunk.memory[0] == -1) {
		return false;
	}
	json_value* json = json_parse(m_chunk.memory, strlen(m_chunk.memory));
	int n = json->u.array.length;
	bool bFound = false;
	int i = 0;
	int l = 0;
	int m = 0;
	for (m = 0; m < n; m++) {
		l = json->u.array.values[m]->u.object.length;
		for (i = 0; i < l; i++) {
			if (strcmp(json->u.array.values[0]->u.object.values[i].name, "Files") == 0) {
				bFound = true;
				break;
			}
		}
		if (bFound) {
			break;
		}
	}
	if (bFound == false) {
		return false;;
	}
	l = json->u.array.values[0]->u.object.values[i].value->u.array.length;
	bool bFoundExt = false;
	bool bFoundLink = false;
	int j = 0;
	for (j = 0; j < l; j++) {
		n = json->u.array.values[0]->u.object.values[i].value->u.array.values[j]->u.object.length;
		for (m = 0; m < n; m++) {
			if (strcmp(json->u.array.values[0]->u.object.values[i].value->u.array.values[j]->u.object.values[m].name, "Ext") == 0) {
				bFoundExt = true;
				strcpy_s(szExt, 16, json->u.array.values[0]->u.object.values[i].value->u.array.values[j]->u.object.values[m].value->u.string.ptr);
			}
			else if (strcmp(json->u.array.values[0]->u.object.values[i].value->u.array.values[j]->u.object.values[m].name, "Link") == 0) {
				bFoundLink = true;
				strcpy_s(szLink, 1024, json->u.array.values[0]->u.object.values[i].value->u.array.values[j]->u.object.values[m].value->u.string.ptr);
			}
			if (bFoundExt && bFoundLink) {
				bFound = true;
				break;
			}
		}
		if (bFound) {
			break;
		}
		bFoundExt = false;
		bFoundLink = false;
	}
	if (bFound == false) {
		return false;
	}
	return true;
}

bool CShooterAPI::DownloadSubFile(const char* szFilename, const char* szURL) {
	free(m_chunk.memory);
	curl_easy_setopt(m_curl, CURLOPT_URL, szURL);
	curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1);
	m_chunk.memory = (char*)malloc(1);
	m_chunk.size = 0;
	if (curl_easy_perform(m_curl) != CURLE_OK) {
		strcpy_s(m_szError, 256, "\nError: Unable to download sub title.\n");
		if (m_chunk.size > 0) {
			free(m_chunk.memory);
		}
		return false;
	}
	FILE* f = NULL;
	fopen_s(&f, szFilename, "wb");
	if (f == NULL) {
		free(m_chunk.memory);
		strcpy_s(m_szError, 256, "\nError: Unable to write sub file.\n");
		return false;
	}
	fwrite(m_chunk.memory, m_chunk.size, 1, f);
	free(m_chunk.memory);
	fclose(f);
	return true;
}

size_t CShooterAPI::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	MEMORY_CHUNK *mem = (MEMORY_CHUNK*)userp;
	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}
