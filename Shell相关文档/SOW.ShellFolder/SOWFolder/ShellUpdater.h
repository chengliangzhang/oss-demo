#pragma once

#include "SOWFileSystem.h"


class CShellUpdater
{
public:
    CShellUpdater(void);
    ~CShellUpdater(void);

public:
    void Initialise();
    void AddItemForUpdate(HWND item);
    void Stop();

private:
    static unsigned int __stdcall ThreadEntry(void* pContext);
    void WorkerThread();

private:
    CComAutoCriticalSection m_critSec;
    HANDLE m_hThread;
    CSimpleValArray<HWND> m_aWnd;
    CSimpleValArray<PIDLIST_RELATIVE> m_aPidl;
    HANDLE m_hTerminationEvent;
    HANDLE m_hWakeEvent;

    bool m_bItemsAddedSinceLastUpdate;
    volatile LONG m_bRunning;
};


