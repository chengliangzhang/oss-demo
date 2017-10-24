/**************************************************************************
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

(c) Microsoft Corporation. All Rights Reserved.
**************************************************************************/

#pragma once

BOOL SplitString(WCHAR *szSource, WCHAR *szDelim, int nMaxSize, int *pnSize, WCHAR **pszOutput, int *pnLength);
int GetResult(WCHAR* szReceive, WCHAR* szRetMsg=NULL, int nMsgLen=0);
void GetCommandResult(WCHAR* szBuffer, int *pnResult, WCHAR* szRetMsg, int nMsgLen);
BOOL StringToSystemTime(WCHAR* szBuffer, int nLength, SYSTEMTIME *pTime);
WCHAR* StripFistPathName(WCHAR* pszPath);
void GetPathName(WCHAR* pszPath, WCHAR* pszBuffer, int nLen);

extern HINSTANCE g_hInst;

#define FIELD_DELIM	L"`#"      // 每条记录中各个字段的分隔符
#define RECORD_DELIM L"~@"     // 记录之间的分隔符

#define COMMAND_GETPROJECT		L"0007"
#define COMMAND_GETFOLDERFILE	L"0019"
#define COMMAND_CHECKFILE		L"0196"
#define COMMAND_GETFILEDATA		L"0197"
#define COMMAND_SENDFILEDATA	L"0198"
#define COMMAND_CREATEFILE		L"0199"
#define COMMAND_COMMITFILE		L"0201"
#define COMMAND_DELETEFILE		L"0031"
#define COMMAND_SETFILEVERSION	L"0203"
#define COMMAND_CREATEFOLDER	L"0021"
#define COMMAND_RENAMEFOLDER	L"0027"
#define COMMAND_RENAMEFILE		L"0131"
#define COMMAND_GetIDBYPIDANDNAME	L"0204"
#define COMMAND_GETFILEBYFULLPATH	L"0206"
#define COMMAND_GETFOLDERBYFULLPATH	L"0208"
#define COMMAND_CHECKIN			L"0170"
#define COMMAND_CHECKOUT		L"0172"
#define COMMAND_SHARE			L"0166"
#define COMMAND_UNSHARE			L"0168"
#define COMMAND_GETCHANGEDFILES L"0210"

struct TPkgHeader 
{
	DWORD nSeq;
	int nBodyLen;
};

struct TPkgBody 
{
	int nStr;			// WCHAR的字符数
	int nByte;			// byte数目
	BYTE data[1];		// data由WCHAR和byte两部分数据组成，大小由nStr和nByte指定,中间有一个‘\0’分隔符
};

struct TPkgInfo
{
	bool bHeader;
	int  nLength;

	TPkgInfo(bool bIsHeader = true, int nLen = sizeof(TPkgHeader)) : bHeader(bIsHeader), nLength(nLen) {}
	void Reset() {bHeader = true, nLength = sizeof(TPkgHeader);}
	~TPkgInfo() {}
};

CBufferPtr* GeneratePkgBuffer(DWORD seq, WCHAR *pStr, int nStr, BYTE* pByte, int nByte);
CBufferPtr* GeneratePkgBuffer(const TPkgHeader& header, const TPkgBody& body);

// 用于与服务器通讯的类(实际是与本地服务代理通讯)
class CMsg
{
public:
	CMsg();
    ~CMsg();

    BOOL Connect();
	BOOL Disconnect();
	BOOL IsConnected();
	int SendMsg(WCHAR* pszSend, int nSendLen, BYTE* pByte, int nByte, WCHAR* pszReceive, int nReceiveLen, BYTE *pRetBuf=NULL, int *pnRetBufLen=NULL, int nTimeout=30, BOOL bRetry=TRUE);

	int GetProjects(WCHAR* pszReceive, int nReceiveLength);
	int GetFolderContent(LPCWSTR pszProjectID, LPCWSTR pszParentID, LPCWSTR pszSysPath, WCHAR* pszReceive, int nReceiveLength);
	int GetFileData(LPCWSTR pszMD5, long nStart, int nLength, WCHAR* pszReceive, int nReceiveLength, BYTE* pszByte, int *pnByteLen);
	int SendFileData(LPCWSTR pszID, long nStart, BYTE* pszData, int nByte, WCHAR* pszReceive, int nReceiveLength);
	int CreateEmptyFile(LPCWSTR pszFolderID, LPCWSTR pszFileID, LPCWSTR pszFileName, WCHAR* pszReceive, int nReceiveLength);
	int CommitFile(LPCWSTR pszFolderID, LPCWSTR pszProjectID, LPCWSTR pszFileID, LPCWSTR pszFileName, WCHAR* pszReceive, int nReceiveLength);
	int DeleteFile(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength);
	int SetFileVersion(LPCWSTR pszProjectID, LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength);
	int CheckFile(LPCWSTR pszProjectID, LPCWSTR pszFileID, LPCWSTR pszOverwrite, WCHAR* pszReceive, int nReceiveLength, WCHAR* pszFile, int nFileLen, BOOL *pbUpdate=NULL, BOOL *pbModified=NULL, BOOL *pbIllegal=NULL);
	int CreateFolder(LPCWSTR pszParentID, LPCWSTR pszName, WCHAR* pszReceive, int nReceiveLength);
	int ChangeFolderName(LPCWSTR pszID, LPCWSTR pszNewName, WCHAR* pszReceive, int nReceiveLength);
	int ChangeFileName(LPCWSTR pszID, LPCWSTR pszNewName, WCHAR* pszReceive, int nReceiveLength);
	int GetIDByParentIDAndName(LPCWSTR pszFolderID, LPCWSTR pszName, LPCWSTR pszFlag, WCHAR* pszReceive, int nReceiveLength);
	int GetFileByFullPath(LPCWSTR pszFullPath, WCHAR* pszReceive, int nReceiveLength);
	int GetFolderByFullPath(LPCWSTR pszFullPath, WCHAR* pszReceive, int nReceiveLength);
	int Checkout(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength);
	int Checkin(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength);
	int ShareFile(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength);
	int UnshareFile(LPCWSTR pszFileID, WCHAR* pszReceive, int nReceiveLength);
	int GetChangedFiles(WCHAR* pszReceive, int nReceiveLength);

private:
	TPkgInfo m_pkgInfo;
	CBufferPtr *m_pBuffer;
	HANDLE m_hAns;
	BOOL m_bSend;

	HP_TcpPullClient m_pClient;
	HP_TcpPullClientListener m_pListener;

	static CMsg* m_spThis;
	static En_HP_HandleResult __stdcall OnConnect(HP_Client pClient);
	static En_HP_HandleResult __stdcall OnSend(HP_Client pClient, const BYTE* pData, int iLength);
	static En_HP_HandleResult __stdcall OnReceive(HP_Client pClient, int iLength);
	static En_HP_HandleResult __stdcall OnClose(HP_Client pClient);
	static En_HP_HandleResult __stdcall OnError(HP_Client pClient, En_HP_SocketOperation enOperation, int iErrorCode);
};
