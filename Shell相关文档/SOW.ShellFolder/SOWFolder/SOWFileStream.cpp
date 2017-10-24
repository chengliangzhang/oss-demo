
#include "stdafx.h"

#include "SOWFileSystem.h"
#include "ShellFolder.h"
#include "Utils.h"

extern CMsg g_Msg;

///////////////////////////////////////////////////////////////////////////////
// CSOWFileStream

CSOWFileStream::CSOWFileStream(const VFS_STREAM_REASON& Reason, CSOWFileItem* pItem, CShellFolder* pFolder) :
	m_Reason(Reason), 
	m_dwCurPos(0), 
	m_dwFileSize(0), 
	m_spFolder(pFolder), 
	m_spItem(pItem)
{
	if (pItem)
		memcpy(&m_data, pItem->m_pData, sizeof(SOWITEMPIDLINFO));
	else
		memset(&m_data, 0, sizeof(SOWITEMPIDLINFO));
}

CSOWFileStream::~CSOWFileStream()
{
	Close();
}

HRESULT CSOWFileStream::Init()
{
	if (m_Reason.uAccess == GENERIC_READ)
	{
		m_dwFileSize = m_data.nSize;
		return S_OK;
	}
	else
	{
		int nRet;
		WCHAR szReceive[256] = {0};
		nRet = g_Msg.CreateEmptyFile(m_data.szParentID, m_data.szID, m_data.szName, szReceive, lengthof(szReceive));
		if (nRet == 0)
		{
			nRet = GetResult(szReceive);
			if (nRet == 0)
				return S_OK;
			else
				return E_FAIL;
		}
		else
			return E_FAIL;
	}
}

HRESULT CSOWFileStream::Seek(DWORD dwPos)
{
	//if (m_Reason.uAccess == GENERIC_WRITE) return E_ACCESSDENIED;
	if (dwPos > m_dwFileSize) dwPos = m_dwFileSize;
	m_dwCurPos = dwPos;
	return S_OK;
}

HRESULT CSOWFileStream::GetCurPos(DWORD* pdwPos)
{
	*pdwPos = m_dwCurPos;
	return S_OK;
}

HRESULT CSOWFileStream::GetFileSize(DWORD* pdwFileSize)
{
	*pdwFileSize = m_dwFileSize;
	return S_OK;
}

HRESULT CSOWFileStream::SetFileSize(DWORD dwSize)
{
	m_dwFileSize = dwSize;
	return S_OK;
}

HRESULT CSOWFileStream::Read(LPVOID pData, ULONG dwSize, ULONG& dwBytesRead)
{
	WCHAR szReceive[256]={0};
	int nRead = dwSize;
	int nRet = g_Msg.GetFileData(m_data.szMD5, m_dwCurPos, (int)dwSize, szReceive, lengthof(szReceive), (BYTE*)pData, &nRead);
	if (nRet == 0 && GetResult(szReceive) == 0)
	{
		dwBytesRead = nRead;
		m_dwCurPos += nRead;
		return S_OK;
	}
	else
		return E_FAIL;
}

HRESULT CSOWFileStream::Write(LPCVOID pData, ULONG dwSize, ULONG& dwBytesWritten)
{
	WCHAR szReceive[256]={0};
	int nRet = g_Msg.SendFileData(m_data.szID, m_dwCurPos, (BYTE*)pData, dwSize, szReceive, lengthof(szReceive));
	if (nRet == 0 && GetResult(szReceive) == 0)
	{
		dwBytesWritten = dwSize;
		m_dwCurPos += dwSize;
		return S_OK;
	}
	else
		return E_FAIL;
}

HRESULT CSOWFileStream::Commit()
{
	WCHAR szReceive[256]={0};
	int nRet = g_Msg.CommitFile(m_data.szParentID, m_data.szProjectID, m_data.szID, m_data.szName, szReceive, lengthof(szReceive));
	if (nRet == 0 && GetResult(szReceive) == 0)
		return S_OK;
	return E_FAIL;
}

HRESULT CSOWFileStream::Close()
{
	if (m_Reason.uAccess == GENERIC_WRITE)
		::SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, m_spFolder->m_pidlMonitor);
	return S_OK;
}

