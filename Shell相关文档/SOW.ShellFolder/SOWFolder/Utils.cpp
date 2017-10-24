/**************************************************************************
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   (c) Microsoft Corporation. All Rights Reserved.
**************************************************************************/
#include "stdafx.h"
#include "HPSocket4C.h"
#include "BufferPtr.h"
#include <windows.h>
#include <shlobj.h>
#include "resource.h"
#include "Utils.h"
#include "shlwapi.h"
#include "ShUtils.h"

extern CMsg g_Msg;
CRITICAL_SECTION cs;

//使用分隔符szDelim定义的字符串分割szSource
//为了避免频繁分配和回收内存,这里通过传入固定大小的数组来接收结果,调用程序通过字符串指针和长度来进行串复制,才能得到正确的结果
//nMaxSize: 数组pszOutput和pnLength的最大长度
//pnSize: 结果个数; pszOutput: 每个分割后的字符串的开始位置的指针  pnLength: 每个分割后的字符串的长度
BOOL SplitString(WCHAR *szSource, WCHAR *szDelim, int nMaxSize, int *pnSize, WCHAR **pszOutput, int *pnLength)
{
	if (szSource == NULL || szDelim == NULL || pnSize == NULL || pszOutput == NULL)
		return FALSE;
	if (wcslen(szSource) == 0 || wcslen(szDelim) == 0)
		return FALSE;
	*pnSize = 0;

	int nLength = (int)wcslen(szDelim);
	WCHAR *szValid = szSource;
	
	WCHAR *p = wcsstr(szSource, szDelim);
	// 去掉开头的分隔符
	if (p != NULL && wcslen(p) == wcslen(szSource))
	{
		szValid = p + nLength;
	}

	p = szValid;
	int nCount = 0;
    while (p && *p != '\0')
    {
		// 分隔符在字符串结尾也不计数
		WCHAR *p1 = wcsstr(p, szDelim);
		if (p1 && wcslen(p1) == nLength && wcscmp(p1, szDelim) == 0)
			break;
		p = p1;
        if (p)
		{
            nCount++;
			p += nLength;
		}
    }
	*pnSize = min(nCount+1, nMaxSize);
	p = szValid;
	nCount = 0;
	while (p && *p != '\0')
	{
		if (nCount + 1 > nMaxSize)
			break;
		WCHAR *p1 = wcsstr(p, szDelim);
		if (p1 == NULL)
		{
			pszOutput[nCount] = p;
			pnLength[nCount] = (int)wcslen(p);
			nCount++;
			break;
		}
		else
		{
			pszOutput[nCount] = p;
			pnLength[nCount] = (int)(p1 - p);
			p = p1 + nLength;
			nCount++;
		}
	}
	return TRUE;
}

//直接从整个返回的缓冲区获得结果（第一行记录）
int GetResult(WCHAR* szReceive, WCHAR* szRetMsg, int nMsgLen)
{
	int nRet;
	WCHAR* pszRecord[2] = {0};
	WCHAR szBuffer[256] = {0};
	int pnRecord[2];
	int nRecordSize;
	if (SplitString(szReceive, RECORD_DELIM, 256, &nRecordSize, pszRecord, pnRecord))
	{
		if (nRecordSize == 0)
			return -1;
		// 第一行记录为返回值及返回信息；其余行为记录信息
		memset(szBuffer, 0, sizeof(szBuffer));
		wcsncpy_s(szBuffer, pszRecord[0], min(pnRecord[0],lengthof(szBuffer)-1));
		GetCommandResult(szBuffer, &nRet, szRetMsg, nMsgLen);
		return nRet;
	}
	return -1;
}


//从返回的第一行记录获得调用结果
void GetCommandResult(WCHAR* szBuffer, int *pnResult, WCHAR* szRetMsg, int nMsgLen)
{
	WCHAR szResult[2]={0};
	WCHAR* szOutput[2];
	int pnLength[2];
	int nSize;
	if (SplitString(szBuffer, FIELD_DELIM, 2, &nSize, szOutput, pnLength))
	{
		if (nSize >= 1)
		{
			wcsncpy_s(szResult, szOutput[0], min(pnLength[0],lengthof(szResult)-1));
			if (pnResult)
				*pnResult = wcscmp(szResult, L"1") == 0 ? 0 : -1;
			if (nSize >= 2 && szRetMsg)
				wcsncpy_s(szRetMsg, nMsgLen, szOutput[1], min(pnLength[1],nMsgLen-1));
		}
	}
}

// 去掉路径中的第一个
WCHAR* StripFistPathName(WCHAR* pszPath)
{
	WCHAR* p = wcschr(pszPath, '\\');
	if (p)
		return p + 1;
	else
		return pszPath;
}

// 获取路径名
void GetPathName(WCHAR* pszPath, WCHAR* pszBuffer, int nLen)
{
	WCHAR* p = wcsrchr(pszPath, '\\');
	if (p)
	{
		size_t nSize = p - pszPath;
		wcsncpy_s(pszBuffer, nLen, pszPath, min(nSize, (size_t)nLen-1));
	}
	else
	{
		wcsncpy_s(pszBuffer, nLen, pszPath, min(wcslen(pszPath), (size_t)nLen - 1));
	}
}

BOOL StringToSystemTime(WCHAR* szBuffer, int nLength, SYSTEMTIME *pTime)
{
	if (szBuffer == NULL || nLength == 0 || pTime == NULL)
		return FALSE;
	memset(pTime, 0, sizeof(SYSTEMTIME));
	if (nLength == 8)
		swscanf_s(szBuffer, L"%04d%02d%02d", &pTime->wYear, &pTime->wMonth, &pTime->wDay, nLength);
	else if (nLength == 10)
		swscanf_s(szBuffer, L"%04d-%02d-%02d", &pTime->wYear, &pTime->wMonth, &pTime->wDay, nLength);
	else if (nLength == 14)
		swscanf_s(szBuffer, L"%04d%02d%02d%02d%02d%02d", &pTime->wYear, &pTime->wMonth, &pTime->wDay, &pTime->wHour, &pTime->wMinute, &pTime->wSecond, nLength);
	else if (nLength == 17)
		swscanf_s(szBuffer, L"%04d%02d%02d%02d%02d%02d", &pTime->wYear, &pTime->wMonth, &pTime->wDay, &pTime->wHour, &pTime->wMinute, &pTime->wSecond, nLength);
	else if (nLength == 19)
		swscanf_s(szBuffer, L"%04d-%02d-%02d %02d:%02d:%02d", &pTime->wYear, &pTime->wMonth, &pTime->wDay, &pTime->wHour, &pTime->wMinute, &pTime->wSecond, nLength);
	else
		return FALSE;
	return TRUE;
}

CBufferPtr* GeneratePkgBuffer(DWORD seq, WCHAR *pStr, int nStr, BYTE* pByte, int nByte)
{
	int nHeaderLen	= sizeof(TPkgHeader);
	int nStrLen		= (nStr+1) * sizeof(WCHAR);
	int nByteLen	= nByte * sizeof(BYTE);
	int nBodyLen	= nStrLen + nByteLen + 2 * sizeof(int);

	CBufferPtr* pBuffer = new CBufferPtr(nHeaderLen + nBodyLen);
	memset(pBuffer->Ptr(), 0, nHeaderLen + nBodyLen);

	TPkgHeader header;
	header.nSeq		= seq;
	header.nBodyLen	= nBodyLen;
	memcpy(pBuffer->Ptr(), (BYTE*)&header, nHeaderLen);

	TPkgBody *pBody = (TPkgBody*)(pBuffer->Ptr()+nHeaderLen);
	pBody->nByte	= nByte;
	pBody->nStr		= nStr;
	if (nStr > 0 && pStr)
		memcpy(pBody->data, pStr, nStr * sizeof(WCHAR));
	if (nByte > 0 && pByte)
		memcpy(pBody->data + nStrLen, pByte, nByteLen);

	return pBuffer;
}

CBufferPtr* GeneratePkgBuffer(const TPkgHeader& header, const TPkgBody& body)
{
	int header_len	= sizeof(TPkgHeader);
	int body_len	= header.nBodyLen;
	
	CBufferPtr* pBuffer = new CBufferPtr(header_len + body_len);

	memcpy(pBuffer->Ptr(), (BYTE*)&header, header_len);
	memcpy(pBuffer->Ptr() + header_len, (BYTE*)&body, body_len);

	return pBuffer;
}

CMsg* CMsg::m_spThis = NULL;

CMsg::CMsg()
{
	m_pListener	= ::Create_HP_TcpPullClientListener();
	m_pClient	= ::Create_HP_TcpPullClient(m_pListener);
	m_pBuffer	= NULL;
	m_hAns		= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_spThis	= this;
	InitializeCriticalSection(&cs);

	::HP_Set_FN_Client_OnConnect(m_pListener, OnConnect);
	::HP_Set_FN_Client_OnSend(m_pListener, OnSend);
	::HP_Set_FN_Client_OnPullReceive(m_pListener, OnReceive);
	::HP_Set_FN_Client_OnClose(m_pListener, OnClose);
	::HP_Set_FN_Client_OnError(m_pListener, OnError);
}

CMsg::~CMsg()
{
	Disconnect();
	if (m_hAns)	CloseHandle(m_hAns);
	::Destroy_HP_TcpPullClient(m_pClient);
	::Destroy_HP_TcpPullClientListener(m_pListener);
	if (m_pBuffer)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
}

En_HP_HandleResult CMsg::OnConnect(HP_Client pClient)
{
	return HP_HR_OK;
}

En_HP_HandleResult CMsg::OnSend(HP_Client pClient, const BYTE* pData, int iLength)
{
	return HP_HR_OK;
}

En_HP_HandleResult CMsg::OnReceive(HP_Client pClient, int iLength)
{
	int required = m_spThis->m_pkgInfo.nLength;
	int remain = iLength;

	while(remain >= required)
	{
		remain -= required;
		CBufferPtr buffer(required);
		En_HP_FetchResult result = ::HP_TcpPullClient_Fetch(pClient, buffer, (int)buffer.Size());
		if(result == HP_FR_OK)
		{
			if (buffer.Ptr() == NULL)
				break;
			if(m_spThis->m_pkgInfo.bHeader)
			{
				TPkgHeader* pHeader = (TPkgHeader*)buffer.Ptr();
				required = pHeader->nBodyLen;
			}
			else
			{
				TPkgBody* pBody = (TPkgBody*)buffer.Ptr();
				required = sizeof(TPkgHeader);
			}
			m_spThis->m_pkgInfo.bHeader	= !m_spThis->m_pkgInfo.bHeader;
			m_spThis->m_pkgInfo.nLength	= required;
		}
		if (remain == 0)
		{
			if (m_spThis->m_pBuffer)
				delete m_spThis->m_pBuffer;
			m_spThis->m_pBuffer = new CBufferPtr(buffer);
			SetEvent(m_spThis->m_hAns);
		}
	}

	return HP_HR_OK;
}

En_HP_HandleResult CMsg::OnClose(HP_Client pClient)
{
	return HP_HR_OK;
}

En_HP_HandleResult CMsg::OnError(HP_Client pClient, En_HP_SocketOperation enOperation, int iErrorCode)
{
	return HP_HR_OK;
}

BOOL CMsg::Connect()
{
	m_pkgInfo.Reset();
	m_bSend = FALSE;
	if(::HP_Client_Start(m_pClient, L"127.0.0.1", 5550, true))
		return TRUE;
	return FALSE;
}

BOOL CMsg::Disconnect()
{
	if (m_pBuffer)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
	if (::HP_Client_GetState(m_pClient) == HP_SS_STARTED)
		return ::HP_Client_Stop(m_pClient);
	return TRUE;
}

BOOL CMsg::IsConnected()
{
	if (::HP_Client_GetState(m_pClient) == HP_SS_STARTED)
		return TRUE;
	if (Connect())
		return TRUE;
	return FALSE;
}

int CMsg::SendMsg(WCHAR* pszSend, int nSendLen, BYTE* pByte, int nByte, WCHAR* pszReceive, int nReceiveLen, BYTE *pRetBuf, int *pnRetBufLen, int nTimeout, BOOL bRetry)
{
	if (pszSend == NULL || pszReceive == NULL)
		return -1;
	if (::HP_Client_GetState(m_pClient) != HP_SS_STARTED)
	{
		if (!Connect())
			return -4;
	}
	if (!bRetry)
	{
		EnterCriticalSection(&cs);
		if (m_bSend)
		{
			LeaveCriticalSection(&cs);
			return -2;
		}
		m_bSend = TRUE;
		LeaveCriticalSection(&cs);
	}
	else
	{
		BOOL bWait = FALSE;
		for (int i = 0; i < 30; i++)
		{
			EnterCriticalSection(&cs);
			if (!m_bSend)
			{
				m_bSend = TRUE;
				bWait = TRUE;
				LeaveCriticalSection(&cs);
				break;
			}
			LeaveCriticalSection(&cs);
			Sleep(1000);
		}
		if (!bWait)
			return -2;
	}
	ResetEvent(m_hAns);
	static DWORD SEQ = 0;

	CBufferPtr *pBuffer = GeneratePkgBuffer(++SEQ, pszSend, nSendLen, pByte, nByte);

	::HP_Client_Send(m_pClient, pBuffer->Ptr(), (int)pBuffer->Size());
	BOOL ret = FALSE;
	if (WaitForSingleObject(m_hAns, nTimeout * 1000) == WAIT_OBJECT_0)
	{
		ret = TRUE;
		if (m_pBuffer)
		{
			TPkgBody* pBody	= (TPkgBody*)(m_pBuffer->Ptr());
			if (pBody->nStr > 0 && pBody->nStr <= nReceiveLen)
			{
				memcpy(pszReceive, pBody->data, pBody->nStr * sizeof(WCHAR));
			}
			if (pBody->nByte >= 0 && pnRetBufLen && pBody->nByte <= *pnRetBufLen)
			{
				*pnRetBufLen = pBody->nByte;
				if (pBody->nByte > 0 && pRetBuf)
					memcpy(pRetBuf, pBody->data + (pBody->nStr+1)*sizeof(WCHAR), pBody->nByte);
			}
			delete m_pBuffer;
			m_pBuffer = NULL;
		}
	}
	delete pBuffer;

	EnterCriticalSection(&cs);
	m_bSend = FALSE;
	LeaveCriticalSection(&cs);

	if(ret)
	{
		return 0;
	}
	return -3;
}

int CMsg::GetProjects(WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	wnsprintf(szSend, lengthof(szSend)-1, COMMAND_GETPROJECT);
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength);
}

int CMsg::GetFolderContent(LPCWSTR pszProjectID, LPCWSTR pszParentID, LPCWSTR pszSysPath, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s%s%s", COMMAND_GETFOLDERFILE, FIELD_DELIM, pszProjectID, FIELD_DELIM, pszParentID, FIELD_DELIM, pszSysPath);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength);
}

// 获取文件的工作目录路径
// pszWriteFlag: "0": 仅返回文件路径; "1": 准备覆盖, 如果原文件存在，则需要删除或移走，以重新下载（如果不移走，重新下载会改名） "2": 准备撤销修改
// pbUpdated: 是否需要重新下载文件（在shell里面下载文件，可以提供实时进度）
// pbModified: 如果本地工作文件存在，返回文件是否修改过
// pbIllegal: 如果本地工作文件存在，返回是否非法修改（即虽然本地有修改，但服务器的版本已更新，和本地的版本号不同）
int CMsg::CheckFile(LPCWSTR pszProjectID, LPCWSTR pszFileID, LPCWSTR pszWriteFlag, WCHAR* pszReceive, int nReceiveLength, WCHAR* pszFile, int nFileLen, BOOL *pbUpdate, BOOL *pbModified, BOOL *pbIllegal)
{
	WCHAR szSend[512];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s%s%s", COMMAND_CHECKFILE, FIELD_DELIM, pszProjectID, FIELD_DELIM, pszFileID, FIELD_DELIM, pszWriteFlag);
	if (FAILED(hr))
		return -1;
	int nRet = SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength);
	if (nRet != 0)
		return nRet;
	WCHAR szBuffer[512] = {0};
	WCHAR szBuffer1[512] = {0};
	WCHAR* pszRecord[2] = {0};
	WCHAR* pszField[10] = {0};
	WCHAR szRetMsg[100] = {0};
	int pnRecord[2];
	int pnField[10];
	int nRecordSize;
	int nFieldSize;
	if (SplitString(pszReceive, RECORD_DELIM, 2, &nRecordSize, pszRecord, pnRecord))
	{
		if (nRecordSize == 0)
			return -1;
		// 第一行记录为返回值及错误信息；其余行为记录信息
		memset(szBuffer, 0, sizeof(szBuffer));
		wcsncpy_s(szBuffer, pszRecord[0], min(pnRecord[0],lengthof(szBuffer)-1));
		GetCommandResult(szBuffer, &nRet, szRetMsg, lengthof(szRetMsg));
		if (nRet != 0)
			return nRet;
		memset(szBuffer, 0, sizeof(szBuffer));
		wcsncpy_s(szBuffer, pszRecord[1], min(pnRecord[1],lengthof(szBuffer)-1));
		if (SplitString(szBuffer, FIELD_DELIM, 10, &nFieldSize, pszField, pnField))
		{
			if (nFieldSize < 5)
				return -1;
			wcsncpy_s(pszFile, nFileLen, pszField[0], min(pnField[0],nFileLen-1));
			BOOL bUpdate = FALSE, bModified = FALSE, bIllegal = FALSE;
			wcsncpy_s(szBuffer1, pszField[2], min(pnField[2],lengthof(szBuffer1)-1));
			if (wcscmp(szBuffer1, L"1") == 0)
				bUpdate = TRUE;
			memset(szBuffer1, 0, sizeof(szBuffer1));
			wcsncpy_s(szBuffer1, pszField[3], min(pnField[3],lengthof(szBuffer1)-1));
			if (wcscmp(szBuffer1, L"1") == 0)
				bModified = TRUE;
			memset(szBuffer1, 0, sizeof(szBuffer1));
			wcsncpy_s(szBuffer1, pszField[4], min(pnField[4],lengthof(szBuffer1)-1));
			if (wcscmp(szBuffer1, L"1") == 0)
				bIllegal = TRUE;
			if (pbUpdate)
				*pbUpdate = bUpdate;
			if (pbModified)
				*pbModified = bModified;
			if (pbIllegal)
				*pbIllegal = bIllegal;

			return 0;
		}
		else
			return -1;
	}
	else
		return -1;
}

int CMsg::GetFileData(LPCWSTR pszMD5, long nStart, int nLength, WCHAR* pszReceive, int nReceiveLength, BYTE* pszByte, int *pnByteLen)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%ld%s%d", COMMAND_GETFILEDATA, FIELD_DELIM, pszMD5, FIELD_DELIM, nStart, FIELD_DELIM, nLength);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, pszByte, pnByteLen);
}

int CMsg::SendFileData(LPCWSTR pszID, long nStart, BYTE* pszData, int nByte, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%ld", COMMAND_SENDFILEDATA, FIELD_DELIM, pszID, FIELD_DELIM, nStart);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), pszData, nByte, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::CreateEmptyFile(LPCWSTR pszFolderID, LPCWSTR pszFileID, LPCWSTR pszFileName, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[512];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s%s%s", COMMAND_CREATEFILE, FIELD_DELIM, pszFileID, FIELD_DELIM, pszFileName, FIELD_DELIM, pszFolderID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::CommitFile(LPCWSTR pszFolderID, LPCWSTR pszProjectID, LPCWSTR pszFileID, LPCWSTR pszFileName, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[512];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s%s%s%s%s", COMMAND_COMMITFILE, FIELD_DELIM, pszFileID, FIELD_DELIM, pszFileName, FIELD_DELIM, pszFolderID, FIELD_DELIM, pszProjectID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::DeleteFile(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_DELETEFILE, FIELD_DELIM, pszFileID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::SetFileVersion(LPCWSTR pszProjectID, LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s", COMMAND_SETFILEVERSION, FIELD_DELIM, pszProjectID, FIELD_DELIM, pszFileID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::CreateFolder(LPCWSTR pszParentID, LPCWSTR pszName, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s", COMMAND_CREATEFOLDER, FIELD_DELIM, pszParentID, FIELD_DELIM, pszName);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::ChangeFolderName(LPCWSTR pszID, LPCWSTR pszNewName, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s", COMMAND_RENAMEFOLDER, FIELD_DELIM, pszID, FIELD_DELIM, pszNewName);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::ChangeFileName(LPCWSTR pszID, LPCWSTR pszNewName, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s", COMMAND_RENAMEFILE, FIELD_DELIM, pszID, FIELD_DELIM, pszNewName);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::GetIDByParentIDAndName(LPCWSTR pszFolderID, LPCWSTR pszName, LPCWSTR pszFlag, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s%s%s%s%s", COMMAND_GetIDBYPIDANDNAME, FIELD_DELIM, pszFolderID, FIELD_DELIM, pszName, FIELD_DELIM, pszFlag);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::GetFileByFullPath(LPCWSTR pszFullPath, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_GETFILEBYFULLPATH, FIELD_DELIM, pszFullPath);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::GetFolderByFullPath(LPCWSTR pszFullPath, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_GETFOLDERBYFULLPATH, FIELD_DELIM, pszFullPath);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::Checkout(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_CHECKOUT, FIELD_DELIM, pszFileID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::Checkin(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_CHECKIN, FIELD_DELIM, pszFileID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::ShareFile(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_SHARE, FIELD_DELIM, pszFileID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::UnshareFile(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s%s%s", COMMAND_UNSHARE, FIELD_DELIM, pszFileID);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL);
}

int CMsg::GetChangedFiles(WCHAR* pszReceive, int nReceiveLength)
{
	WCHAR szSend[256];
	HRESULT hr = wnsprintf(szSend, lengthof(szSend)-1, L"%s", COMMAND_GETCHANGEDFILES);
	if (FAILED(hr))
		return -1;
	return SendMsg(szSend, (int)wcslen(szSend), NULL, 0, pszReceive, nReceiveLength, NULL, NULL, 2, FALSE);
}
