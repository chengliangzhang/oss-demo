#pragma once

#include "NseFileSystem.h"

#define SOW_MAXNAMELEN 200
#define SOW_MAXPATHLEN 255

#define SOW_MAGICID      0xDE

///////////////////////////////////////////////////////////////////////////////
// Definitions
#if !defined(_M_X64) && !defined(_M_IA64)
#include <pshpack1.h>
#endif
#define SOW_FLAG_FOLDER		0x0001
#define SOW_FLAG_READONLY	0x0002
#define SOW_FLAG_CHANGED	0x0004
#define SOW_FLAG_SYSFOLDER	0x0008
#define SOW_FLAG_ALLOWSHARE	0x0010
#define SOW_FLAG_HASOWNER	0x0020

typedef struct tagSOWITEMPIDLINFO
{
   // SHITEMID 
   USHORT cb;
   // Type identifiers
   BYTE magic;		// id
   BYTE reserved;	// for alignment
   DWORD nFlag;		// combination of SOW_FLAG_XXX
   LONG	nSize;
   SYSTEMTIME tmDate;
   WCHAR szName[SOW_MAXNAMELEN];
   WCHAR szProjectID[40];
   WCHAR szParentID[40];
   WCHAR szMD5[40];
   WCHAR szID[40];
   WCHAR szOwner[50];
} SOWITEMPIDLINFO;
#if !defined(_M_X64) && !defined(_M_IA64)
#include <poppack.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// CSOWShellModule

class CSOWShellModule : public CNseModule
{
public:
   // CShellModule

   LONG GetConfigInt(VFS_CONFIG Item);
   BOOL GetConfigBool(VFS_CONFIG Item);
   LPCWSTR GetConfigStr(VFS_CONFIG Item);

   HRESULT DllInstall();
   HRESULT DllUninstall();
   HRESULT ShellAction(LPCWSTR pstrType, LPCWSTR pstrCmdLine);

   BOOL DllMain(DWORD dwReason, LPVOID lpReserved);

   HRESULT CreateFileSystem(PCIDLIST_ABSOLUTE pidlRoot, CNseFileSystem** ppFS);
};


///////////////////////////////////////////////////////////////////////////////
// CSOWFileSystem

class CSOWFileSystem : public CNseFileSystem
{
public:
   volatile LONG m_cRef;                         // Reference count

   // Constructor

   CSOWFileSystem();
   virtual ~CSOWFileSystem();

   // Operations

   HRESULT Init(PCIDLIST_ABSOLUTE pidlRoot);

   // CShellFileSystem
   
   VOID AddRef();
   VOID Release();

   CNseItem* GenerateRoot(CShellFolder* pFolder);
};


///////////////////////////////////////////////////////////////////////////////
// CSOWFileItem

class CSOWFileItem : public CNseBaseItem
{
public:
   const SOWITEMPIDLINFO* m_pData;             // Reference to data inside PIDL item

   CSOWFileItem(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem);
   ~CSOWFileItem();

   // CNseFileItem
   BYTE GetType();
   VFS_FIND_DATA GetFindData();
   SFGAOF GetSFGAOF(SFGAOF Mask);
   HRESULT GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v);
   HRESULT SetProperty(REFPROPERTYKEY pkey, const CComPropVariant& v);

   HRESULT GetColumnInfo(UINT iColumn, VFS_COLUMNINFO& Column);
   HRESULT GetSysIcon(UINT uIconFlags, int* pIconIndex);
   HRESULT GetExtractIcon(REFIID riid, LPVOID* ppRetVal);
   HRESULT GetThumbnail(REFIID riid, LPVOID* ppRetVal);
   HRESULT GetIconOverlay(int* pIconIndex);

   PCITEMID_CHILD GenerateITEMID(const WIN32_FIND_DATA& wfd);
   PCITEMID_CHILD GenerateITEMID(const SOWITEMPIDLINFO& src);

   CNseItem* GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem);
   CNseItem* GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd);

   HRESULT GetChild(LPCWSTR pstrName, SHGNO ParseType, CNseItem** pItem);
   HRESULT EnumChildren(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aList);
   HRESULT GetChildDirect(LPCWSTR pstrFullName, CNseItem** pItem);

   HRESULT GetStream(const VFS_STREAM_REASON& Reason, CNseFileStream** ppFile);

   HRESULT CreateFolder();
   HRESULT Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName);
   HRESULT Delete();

   HMENU GetMenu();
   HRESULT ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd);
   HRESULT IsDropDataAvailable(IDataObject* pDataObj);
   HRESULT SetMenuState(const VFS_MENUSTATE& State);

   // Implementation
   LPCWSTR GetProjectID();
   LPCWSTR GetParentID();
   LPCWSTR GetMD5();
   LPCWSTR GetID();
   LPCWSTR GetOwner();
   LPCWSTR GetName();
   LONG    GetSize();
   DWORD   GetFlag();
   BOOL    ReadOnly();
   BOOL    Changed();
   BOOL    SysFolder();
   BOOL    AllowShare();
   BOOL    HasOwner();
   BOOL    _IsFolder();
   BOOL    OpenFile(HWND hWnd, CShellFolder* pFolder, PCUITEMID_CHILD pidlItem, LPCTSTR pstrVerb=_T("open"));
   BOOL	   SaveFile(HWND hWnd, CShellFolder* pFolder, PCUITEMID_CHILD pidlItem);
   BOOL    RollbackFile(HWND hWnd, CShellFolder* pFolder, PCUITEMID_CHILD pidlItem);
   BOOL    CheckoutFile(HWND hWnd, LPCWSTR pstrFileID);
   BOOL    CheckinFile(HWND hWnd, LPCWSTR pstrFileID);
   BOOL    ShareFile(HWND hWnd, LPCWSTR pstrFileID);
   BOOL    UnshareFile(HWND hWnd, LPCWSTR pstrFileID);
};


///////////////////////////////////////////////////////////////////////////////
// CSOWFileStream

class CSOWFileStream : public CNseFileStream
{
public:
   CSOWFileItem *m_spItem;				         // Reference to file
   CRefPtr<CShellFolder> m_spFolder;			 // Reference to parent folder
   DWORD m_dwCurPos;                             // Current position
   DWORD m_dwFileSize;                           // File Size
   VFS_STREAM_REASON m_Reason;                   // Stream opened for read or write access?
   SOWITEMPIDLINFO m_data;
   // Constructor

   CSOWFileStream(const VFS_STREAM_REASON& Reason, CSOWFileItem* pItem, CShellFolder* pFolder);
   virtual ~CSOWFileStream();

   // CNseFileStream

   HRESULT Init();
   HRESULT Read(LPVOID pData, ULONG dwSize, ULONG& dwRead);
   HRESULT Write(LPCVOID pData, ULONG dwSize, ULONG& dwWritten);
   HRESULT Seek(DWORD dwPos);
   HRESULT GetCurPos(DWORD* pdwPos);
   HRESULT GetFileSize(DWORD* pdwFileSize);
   HRESULT SetFileSize(DWORD dwSize);
   HRESULT Commit();
   HRESULT Close();
};

