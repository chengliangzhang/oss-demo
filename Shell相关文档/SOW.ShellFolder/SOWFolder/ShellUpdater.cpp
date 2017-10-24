#include "stdafx.h"
#include "ShellUpdater.h"
#pragma warning(push)
#pragma warning(disable: 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <shlobj.h>
#pragma warning(pop)
#include "ShUtils.h"
#include "Utils.h"

extern CMsg g_Msg;

typedef CComCritSecLock<CComAutoCriticalSection> AutoLocker;

CShellUpdater::CShellUpdater(void)
{
    m_hWakeEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    m_hTerminationEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    m_bRunning = FALSE;
    m_bItemsAddedSinceLastUpdate = false;
	m_hThread = NULL;
}

CShellUpdater::~CShellUpdater(void)
{
    Stop();
}

void CShellUpdater::Stop()
{
    InterlockedExchange(&m_bRunning, FALSE);
    if (m_hTerminationEvent)
    {
        SetEvent(m_hTerminationEvent);
        if(WaitForSingleObject(m_hThread, 200) != WAIT_OBJECT_0)
        {
            ATLTRACE(L"Error terminating shell updater thread\n");
        }
    }
    CloseHandle(m_hThread);
    CloseHandle(m_hTerminationEvent);
    CloseHandle(m_hWakeEvent);
}

void CShellUpdater::Initialise()
{
    // Don't call Initialize more than once
	if (m_hThread == NULL)
	{
		// Just start the worker thread.
		// It will wait for event being signaled.
		// If m_hWakeEvent is already signaled the worker thread
		// will behave properly (with normal priority at worst).

		InterlockedExchange(&m_bRunning, TRUE);
		unsigned int threadId = 0;
		m_hThread = (HANDLE)_beginthreadex(NULL,0,ThreadEntry,this,0,&threadId);
		SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST);
	}
}

void CShellUpdater::AddItemForUpdate(HWND hWnd)
{
    {
        AutoLocker lock(m_critSec);

		// add item to m_aItem
		bool bFound = false;
		int i;
		for(i = 0; i < m_aWnd.GetSize(); i++)
		{
			if (m_aWnd[i] == hWnd)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			m_aWnd.Add(hWnd);
		}
		//bFound = false;
		//for(i = 0; i < m_aPidl.GetSize(); i++)
		//{
		//	PIDLIST_RELATIVE pidlTmp = m_aPidl[i];
		//	SOWITEMPIDLINFO *pDataTmp = (SOWITEMPIDLINFO*)pidlTmp;
		//	SOWITEMPIDLINFO *pData = (SOWITEMPIDLINFO*)pidl;
		//	if (wcscmp(pDataTmp->szID, pData->szID) == 0)
		//	{
		//		bFound = true;
		//		break;
		//	}
		//}
		//if (!bFound)
		//{
		//	PIDLIST_RELATIVE pidlTmp = (PIDLIST_RELATIVE)::ILCloneChild((PCUITEMID_CHILD)pidl);
		//	m_aPidl.Add(pidlTmp);
		//}

        // set this flag while we are synced
        // with the worker thread
        m_bItemsAddedSinceLastUpdate = true;
    }

    SetEvent(m_hWakeEvent);
}


unsigned int CShellUpdater::ThreadEntry(void* pContext)
{
    //CCrashReportThread crashthread;
    ((CShellUpdater*)pContext)->WorkerThread();
    return 0;
}

void CShellUpdater::WorkerThread()
{
    HANDLE hWaitHandles[2];
    hWaitHandles[0] = m_hTerminationEvent;
    hWaitHandles[1] = m_hWakeEvent;

    for(;;)
    {
        DWORD waitResult = WaitForMultipleObjects(_countof(hWaitHandles), hWaitHandles, FALSE, 1000);

        // exit event/working loop if the first event (m_hTerminationEvent)
        // has been signaled or if one of the events has been abandoned
        // (i.e. ~CShellUpdater() is being executed)
        if(waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED_0 || waitResult == WAIT_ABANDONED_0+1)
        {
            // Termination event
            break;
        }
        // wait some time before we notify the shell
        Sleep(50);
		WCHAR szReceive[65536] = {0};
		WCHAR szBuffer[1024] = {0};
		WCHAR szBuffer1[1024] = {0};
		WCHAR* pszRecord[5000] = {0};
		WCHAR* pszField[50] = {0};
		WCHAR szRetMsg[100] = {0};
		int pnRecord[5000];
		int pnField[50];
		int nRecordSize;
		int nFieldSize;
		int nRet = -1;

        if (!m_bRunning)
            continue;
        {
            //AutoLocker lock(m_critSec);
            if(m_aWnd.GetSize() == 0)
            {
                // Nothing left to do
                continue;
            }

			//query client for list of updated files
			memset(szReceive, 0, sizeof(szReceive));
			nRet = g_Msg.GetChangedFiles(szReceive, lengthof(szReceive));
			if (nRet == 0)
			{
				if (SplitString(szReceive, RECORD_DELIM, 5000, &nRecordSize, pszRecord, pnRecord))
				{
					if (nRecordSize == 0)
						continue;
					// 第一行记录为返回值及错误信息；其余行为记录信息
					memset(szBuffer, 0, sizeof(szBuffer));
					wcsncpy_s(szBuffer, pszRecord[0], min(pnRecord[0],lengthof(szBuffer)-1));
					GetCommandResult(szBuffer, &nRet, szRetMsg, lengthof(szRetMsg));
					if (nRet == 0)
					{
						for (int i = 1; i < nRecordSize; i++)
						{
							memset(szBuffer, 0, sizeof(szBuffer));
							wcsncpy_s(szBuffer, pszRecord[i], min(pnRecord[i],lengthof(szBuffer)-1));
							if (SplitString(szBuffer, FIELD_DELIM, 50, &nFieldSize, pszField, pnField))
							{
								if (nFieldSize == 0)
									continue;
								memset(szBuffer1, 0, sizeof(szBuffer1));
								wcsncpy_s(szBuffer1, pszField[0], min(pnField[0],lengthof(szBuffer1)-1));		// type: folder/file
								if (wcscmp(szBuffer1, L"File") == 0)
								{
									if (nFieldSize < 13)
										continue;
									SOWITEMPIDLINFO data = { 0 };
									wcsncpy_s(data.szID, pszField[1], min(pnField[1],lengthof(data.szID)-1));		// ID
									wcsncpy_s(data.szName, pszField[2], min(pnField[2],lengthof(data.szName)-1));	// Name
									wcsncpy_s(szBuffer1, pszField[3], min(pnField[3],lengthof(szBuffer1)-1));		// File Time
									StringToSystemTime(szBuffer1, (int)wcslen(szBuffer1), &data.tmDate);
									memset(szBuffer1, 0, sizeof(szBuffer1));
									wcsncpy_s(szBuffer1, pszField[4], min(pnField[4],lengthof(szBuffer1)-1));		// File Size
									data.nSize = _wtol(szBuffer1);
									wcsncpy_s(data.szOwner, pszField[7], min(pnField[7],lengthof(data.szOwner)-1));	// Owner
									wcsncpy_s(data.szMD5, pszField[8], min(pnField[8],lengthof(data.szMD5)-1));		// MD5
									wcsncpy_s(data.szProjectID, pszField[11], min(pnField[11],lengthof(data.szProjectID)-1));	// ProjectID
									wcsncpy_s(data.szParentID, pszField[12], min(pnField[12], lengthof(data.szParentID)-1));	// ParentID
									memset(szBuffer1, 0, sizeof(szBuffer1));
									wcsncpy_s(szBuffer1, pszField[5], min(pnField[5],lengthof(szBuffer1)-1));		// Allow Share
									if (wcscmp(szBuffer1, L"1") == 0)
										data.nFlag |= SOW_FLAG_ALLOWSHARE;
									memset(szBuffer1, 0, sizeof(szBuffer1));
									wcsncpy_s(szBuffer1, pszField[6], min(pnField[6],lengthof(szBuffer1)-1));		// Has Owner
									if (wcscmp(szBuffer1, L"1") == 0)
										data.nFlag |= SOW_FLAG_HASOWNER;
									memset(szBuffer1, 0, sizeof(szBuffer1));
									wcsncpy_s(szBuffer1, pszField[9], min(pnField[9],lengthof(szBuffer1)-1));		// Changed
									if (wcscmp(szBuffer1, L"1") == 0)
										data.nFlag |= SOW_FLAG_CHANGED;
									memset(szBuffer1, 0, sizeof(szBuffer1));
									wcsncpy_s(szBuffer1, pszField[10], min(pnField[10],lengthof(szBuffer1)-1));		// ReadOnly
									if (wcscmp(szBuffer1, L"1") == 0)
										data.nFlag |= SOW_FLAG_READONLY;
									data.nFlag &= ~SOW_FLAG_FOLDER;
								    data.magic = SOW_MAGICID;

									PIDLIST_RELATIVE pidl = (PIDLIST_RELATIVE)CNseBaseItem::GenerateITEMID(&data, sizeof(data));
									//PIDLIST_RELATIVE pidl = NULL;
									//for(int n = 0; n < m_aPidl.GetSize(); n++)
									//{
									//	PIDLIST_RELATIVE pidlTmp = m_aPidl[n];
									//	SOWITEMPIDLINFO *pDataTmp = (SOWITEMPIDLINFO*)pidlTmp;
									//	if (wcscmp(pDataTmp->szID, data.szID) == 0)
									//	{
									//		pidl = pidlTmp;
									//		break;
									//	}
									//}
									//if (pidl == NULL)
									//	continue;
									//PIDLIST_RELATIVE pidl1 = (PIDLIST_RELATIVE)CNseBaseItem::GenerateITEMID(&data, sizeof(data));

									for (int n = m_aWnd.GetSize() - 1; n >= 0; n--)
									{
										HWND hWnd = m_aWnd[n];
										if (!::IsWindow(hWnd))
										{
											m_aWnd.RemoveAt(n);
											continue;
										}
										PIDLIST_RELATIVE pidlTmp = ::ILCloneChild((PCUITEMID_CHILD)pidl);

										PIDLIST_RELATIVE ppidl[2] = {pidl, pidlTmp};
										if (ShellFolderView_UpdateObject(hWnd, ppidl) == -1)
										{
											::ILFree(pidlTmp);
										}
									}
									::ILFree(pidl);
								}
							}
						}
					}
					else
					{
						{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"Get changed files, ret=%d,err msg=%s\n", nRet, szRetMsg); OutputDebugString(szOutput);}
					}
				}
			}
			else
			{
				{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"Get changed files, ret=%d\n", nRet); OutputDebugString(szOutput);}
			}
        }
        //if (workingPath.IsEmpty())
        //    continue;
        //CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": shell notification for %s\n", workingPath.GetWinPath());
        //if (workingPath.IsDirectory())
        //{
        //    // first send a notification about a sub folder change, so explorer doesn't discard
        //    // the folder notification. Since we only know for sure that the subversion admin
        //    // dir is present, we send a notification for that folder.
        //    CString admindir = workingPath.GetWinPathString() + L"\\" + g_SVNAdminDir.GetAdminDirName();
        //    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, (LPCTSTR)admindir, NULL);
        //    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
        //    // Sending an UPDATEDIR notification somehow overwrites/deletes the UPDATEITEM message. And without
        //    // that message, the folder overlays in the current view don't get updated without hitting F5.
        //    // Drawback is, without UPDATEDIR, the left tree view isn't always updated...

        //    //SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
        //}
        //else
        //    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, workingPath.GetWinPath(), NULL);
    }
    _endthread();
}

