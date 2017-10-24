
#include "stdafx.h"

#include "SOWFileSystem.h"
#include "ShellFolder.h"
#include "LaunchFile.h"
#include "EnumIDList.h"
#include "ImageThumbProvider.h"
#include "Utils.h"
#include "ShellUpdater.h"
#include "resource.h"
#pragma comment(lib, "Rpcrt4.lib")

extern CMsg g_Msg;
extern CShellUpdater g_Updater;
///////////////////////////////////////////////////////////////////////////////
// CSOWFileItem
int snItem = 0;

CSOWFileItem::CSOWFileItem(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) :
   CNseBaseItem(pFolder, pidlFolder, pidlItem, bReleaseItem)
{
   if (pidlItem == NULL) 
   {
      // Create empty root item
      static SOWITEMPIDLINFO s_Info = { 0 };
      s_Info.magic = SOW_MAGICID;
	  s_Info.nFlag = SOW_FLAG_FOLDER|SOW_FLAG_SYSFOLDER|SOW_FLAG_READONLY;
      m_pData = &s_Info;
   }
   else 
   {
      // Extract item data
      m_pData = reinterpret_cast<const SOWITEMPIDLINFO*>(pidlItem);
   }
   snItem++;
   //ATLTRACE(L"----------------CSOWFileItem ctor=%d, release=%d\n--------------", snItem, bReleaseItem);
}

CSOWFileItem::~CSOWFileItem()
{
   snItem--;
   //ATLTRACE(L"----------------CSOWFileItem dtor=%d\n--------------", snItem);
}

/**
 * Get the item type.
 */
BYTE CSOWFileItem::GetType()
{
   return SOW_MAGICID;   
}

/**
 * Return SFGAOF flags for this item.
 * These flags tell the Shell about the capabilities of this item. Here we
 * can toggle the ability to copy/delete/rename an item.
 */
SFGAOF CSOWFileItem::GetSFGAOF(SFGAOF dwMask)
{
	if (_IsFolder())
	{
		if (ReadOnly() || SysFolder())
			return 
			  SFGAO_FOLDER
			| SFGAO_BROWSABLE
			| SFGAO_CANCOPY
			| SFGAO_CANLINK
			| SFGAO_HASSUBFOLDER;
		else
			return
			  SFGAO_FOLDER
			| SFGAO_BROWSABLE
			| SFGAO_CANCOPY
			| SFGAO_CANLINK
			| SFGAO_HASSUBFOLDER
			| SFGAO_CANMOVE
			| SFGAO_CANDELETE
			| SFGAO_CANRENAME;
	}
	else
	{
		if (ReadOnly())
		   return   
				  SFGAO_CANCOPY
				| SFGAO_CANLINK
				| SFGAO_STREAM; 
		else
			return
				  SFGAO_CANCOPY
				| SFGAO_CANLINK
				| SFGAO_STREAM 
				| SFGAO_CANMOVE
				| SFGAO_CANDELETE
				| SFGAO_CANRENAME;
	}
}

/**
 * Get information about column definition.
 * We return details of a column requested, plus information about the 
 * combined set of properties supported by items contained in this folders. We 
 * decide which columns to show by default here too.
 */
HRESULT CSOWFileItem::GetColumnInfo(UINT iColumn, VFS_COLUMNINFO& Column)
{
   static VFS_COLUMNINFO aColumns[] = 
   {
      { PKEY_ItemNameDisplay,            SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT,                    0 },
      { PKEY_DateModified,               SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT,                    0 },
      { PKEY_Size,                       SHCOLSTATE_TYPE_INT  | SHCOLSTATE_ONBYDEFAULT,                    0 },
      { PKEY_FileOwner,	                 SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT,                    0 },
      { PKEY_ItemPathDisplay,            SHCOLSTATE_TYPE_STR  | SHCOLSTATE_SECONDARYUI,                    0 },
      { PKEY_FileAttributes,             SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ItemType,                   SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ItemTypeText,               SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ItemPathDisplayNarrow,      SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },      
      { PKEY_SFGAOFlags,                 SHCOLSTATE_TYPE_INT  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_Volume_IsRoot,              SHCOLSTATE_TYPE_INT  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_InfoTip,           SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_TileInfo,          SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
   };
   if (iColumn >= lengthof(aColumns)) return E_FAIL;
   Column = aColumns[iColumn];
   return S_OK;
}

/**
 * Return item information.
 */
HRESULT CSOWFileItem::GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v)
{
   if (pkey == PKEY_Volume_FileSystem) 
   {
      return ::InitPropVariantFromString(L"SOWFS", &v);
   }
   if (pkey == PKEY_ParsingName) 
   {
      return ::InitPropVariantFromString(GetName(), &v);
   } 
   if (pkey == PKEY_ItemName) 
   {
      return ::InitPropVariantFromString(GetName(), &v);
   }
   if (pkey == PKEY_ItemNameDisplay) 
   {
      return ::InitPropVariantFromString(GetName(), &v);
   }
   if (pkey == PKEY_FileOwner)
   {
      return ::InitPropVariantFromString(GetOwner(), &v);
   }
   if (pkey == PKEY_DateModified) 
   {
      const WIN32_FIND_DATA wfd = GetFindData();
      return ::InitPropVariantFromFileTime(&wfd.ftLastWriteTime, &v);
   }
   if (pkey == PKEY_Size) 
   {
      if (!_IsFolder())
	  {
		  const WIN32_FIND_DATA wfd = GetFindData();
		  return ::InitPropVariantFromUInt64(wfd.nFileSizeLow, &v);
	  }
   }
      // Returning file-attributes allow keys to be sorted/displayed
   // before values in the list.
   if (pkey == PKEY_FileAttributes) 
   {
      if (_IsFolder())
	      return ::InitPropVariantFromUInt32(FILE_ATTRIBUTE_DIRECTORY, &v);
	  else
		  return ::InitPropVariantFromUInt32(FILE_ATTRIBUTE_NORMAL, &v);
   }
   // Return properties for display details. These define what properties to
   // display in various places in the Shell, such as the hover-tip and Details panel.
   if (pkey == PKEY_FileName) 
   {
      return ::InitPropVariantFromString(GetName(), &v);
   }
   if (pkey == PKEY_Volume_IsRoot) 
   {
      return ::InitPropVariantFromBoolean(IsRoot(), &v);
   }
   if (pkey == PKEY_PropList_TileInfo) 
   {
      return ::InitPropVariantFromString(L"prop:System.ItemTypeText;", &v);
   }
   if (pkey == PKEY_PropList_ExtendedTileInfo) 
   {
      return ::InitPropVariantFromString(L"prop:System.ItemTypeText;System.DateModified;", &v);
   }
   if (pkey == PKEY_PropList_PreviewTitle) 
   {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;System.ItemTypeText;", &v);
   }
   if (pkey == PKEY_PropList_PreviewDetails) 
   {
      return ::InitPropVariantFromString(L"prop:System.DateModified;System.Title;System.Comment;System.Keywords;", &v);
   }
   if (pkey == PKEY_PropList_InfoTip) 
   {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;System.ItemTypeText;System.DateModified;System.Comment;System.Keywords;", &v);
   }
   if (pkey == PKEY_PropList_FullDetails) 
   {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;System.ItemTypeText;System.DateModified;System.Title;System.Comment;System.Keywords;", &v);
   }
   return CNseBaseItem::GetProperty(pkey, v);
}

/**
 * Set item information.
 */
HRESULT CSOWFileItem::SetProperty(REFPROPERTYKEY pkey, const CComPropVariant& v)
{
   return S_OK;
}

/**
 * Get system icon index.
 * This is slightly faster for the Shell than using GetExtractIcon().
 * Return S_FALSE if no system index exists.
 */
HRESULT CSOWFileItem::GetSysIcon(UINT uIconFlags, int* pIconIndex)
{
   // Use our SHGetFileSysIcon() method to get the System Icon index
   if (_IsFolder())
	   return ::SHGetFileSysIcon(GetName(), FILE_ATTRIBUTE_DIRECTORY, uIconFlags, pIconIndex);
   else
	   return ::SHGetFileSysIcon(GetName(), FILE_ATTRIBUTE_NORMAL, uIconFlags, pIconIndex);
}

HRESULT CSOWFileItem::GetExtractIcon(REFIID riid, LPVOID* ppRetVal)
{
	if (_IsFolder())
		return ::SHCreateFileExtractIcon(GetName(), FILE_ATTRIBUTE_DIRECTORY, riid, ppRetVal);
	else
		return ::SHCreateFileExtractIcon(GetName(), FILE_ATTRIBUTE_NORMAL, riid, ppRetVal);
}

/**
 * Get system icon overlay index.
 * Return the System Icon Overlay index for the item.
 * This feature requires the VFS_HAVE_ICONOVERLAYS configuration flag.
 * Return S_FALSE if no overlay is used.
 */
HRESULT CSOWFileItem::GetIconOverlay(int* pIconIndex)
{
	if (pIconIndex)
	{
		if (!_IsFolder())
		{
			if (Changed())
			{
				*pIconIndex = 14;
				return S_OK;
			}
		}
	}
	return S_FALSE;
}

HRESULT CSOWFileItem::GetThumbnail(REFIID riid, LPVOID* ppRetVal)
{
	return E_NOTIMPL;
   //return ::SHCreateImageThumbProvider(m_pFolder, GetITEMID(), riid, ppRetVal);
}

/**
 * Return file information.
 * We use this to return a simple structure with key information about
 * our item.
 */
VFS_FIND_DATA CSOWFileItem::GetFindData()
{
   VFS_FIND_DATA wfd = { 0 };
   wcscpy_s(wfd.cFileName, lengthof(wfd.cFileName), GetName());
   if (m_pData)
   {
	   if (!_IsFolder())
	   {
		   FILETIME FileTime;
		   SystemTimeToFileTime(&m_pData->tmDate, &FileTime);
		   LocalFileTimeToFileTime(&FileTime, &wfd.ftLastWriteTime);
		   wfd.nFileSizeLow = m_pData->nSize;
	   }
   }
   if (_IsFolder())
	   wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL;
   else
	   wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_VIRTUAL;
   return wfd;
}

/**
 * Serialize item from static data.
 * The Shell sometimes uses this method when it wants to create a brand
 * new folder.
 */
PCITEMID_CHILD CSOWFileItem::GenerateITEMID(const WIN32_FIND_DATA& wfd)
{
   // Serialize data from a WIN32_FIND_DATA structure to a child PIDL.
   SOWITEMPIDLINFO data = { 0 };
   data.magic = SOW_MAGICID;
   wcscpy_s(data.szName, lengthof(data.szName), wfd.cFileName);
   if (wfd.ftLastWriteTime.dwLowDateTime == 0)
	   GetLocalTime(&data.tmDate);
   else
   {
	   FILETIME LocalFileTime;
	   FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &LocalFileTime);
       FileTimeToSystemTime(&LocalFileTime, &data.tmDate);
   }
   if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	   data.nFlag |= SOW_FLAG_FOLDER;
   else
	   data.nFlag &= ~SOW_FLAG_FOLDER;
   if (m_pData)
   {
	   wcsncpy_s(data.szProjectID, m_pData->szProjectID, min(wcslen(m_pData->szProjectID),lengthof(data.szProjectID)-1));
	   wcsncpy_s(data.szParentID, m_pData->szID, min(wcslen(m_pData->szID),lengthof(data.szParentID)-1));
	   if (wfd.cAlternateFileName[1] == VFS_HACK_SAVEAS_JUNCTION)
	   {
		   // this file may exist, query first
		   WCHAR szReceive[512]={0};
		   WCHAR szID[40]={0};
		   WCHAR szFlag[10]=L"";
		   if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			   wcscpy(szFlag, L"Folder");
		   else
			   wcscpy(szFlag, L"File");
		   int nRet = g_Msg.GetIDByParentIDAndName(GetID(), data.szName, szFlag, szReceive, lengthof(szReceive));
		   if (nRet == 0 && GetResult(szReceive, szID, lengthof(szID)) == 0)
		   {
			   wcsncpy_s(data.szID, szID, min(wcslen(szID),lengthof(data.szID)-1));		// ID
		   }
	   }
	   if (wcslen(data.szID) == 0)
	   {
		   GUID guid;
		   CoCreateGuid(&guid);
		   WCHAR* wszUuid = NULL;
		   UuidToString(&guid, (RPC_WSTR*)&wszUuid);
		   if (wszUuid != NULL)
		   {
			   wcsncpy_s(data.szID, wszUuid, min(wcslen(wszUuid),lengthof(data.szID)-1));
			   // free up the allocated string
			   ::RpcStringFreeW((RPC_WSTR*)&wszUuid);
			   wszUuid = NULL;
		   }
	   }
   }
   return CNseBaseItem::GenerateITEMID(&data, sizeof(data));
}

/**
 * Serialize item from static data.
 */
PCITEMID_CHILD CSOWFileItem::GenerateITEMID(const SOWITEMPIDLINFO& src)
{
   // Serialize data from our SOWITEMPIDLINFO structure to a child PIDL.
   SOWITEMPIDLINFO data = src;
   data.magic = SOW_MAGICID;
   return CNseBaseItem::GenerateITEMID(&data, sizeof(data));
}

/**
 * Create an NSE Item instance from a child PIDL.
 */
CNseItem* CSOWFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem)
{
   // Use a pidl-wrapper to validate the correctness of the PIDL and spawn
   // an NSE Item based on the type.
   CPidlMemPtr<SOWITEMPIDLINFO> pItem = pidlItem;
   if (pItem.IsType(SOW_MAGICID)) return new CSOWFileItem(pFolder, pidlFolder, pidlItem, bReleaseItem);
   return NULL;
}

/**
 * Create an NSE Item instance from static data.
 */
CNseItem* CSOWFileItem::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd)
{
   return new CSOWFileItem(pFolder, pidlFolder, CSOWFileItem::GenerateITEMID(wfd), TRUE);
}

/**
 * Look up a single item by filename.
 * Validate its existance outside any cache.
 */
HRESULT CSOWFileItem::GetChild(LPCWSTR pwstrName, SHGNO ParseType, CNseItem** pItem)
{
	//ATLASSERT(_ShellModule.GetConfigBool(VFS_HAVE_UNIQUE_NAMES));
	//ATLASSERT(*pItem==NULL);
	//ATLASSERT(IsFolder());

	//SOWITEMPIDLINFO data = { 0 };
	//WCHAR szReceive[256]={0};
	//WCHAR szID[40]={0};
	//int nRet = g_Msg.GetIDByParentIDAndName(GetID(), pwstrName, L"", szReceive, lengthof(szReceive));
	//if (nRet == 0 && GetResult(szReceive, szID, lengthof(szID)) == 0)
	//{
	//	wcsncpy_s(data.szID, szID, min(wcslen(szID),lengthof(data.szID)-1));		// ID
	//}
	//wcsncpy_s(data.szName, pwstrName, min(wcslen(pwstrName),lengthof(data.szName)-1));	// Name
	//wcsncpy_s(data.szProjectID, GetProjectID(), min(wcslen(GetProjectID()), lengthof(data.szProjectID)-1));
	//wcsncpy_s(data.szParentID, GetID(), min(wcslen(GetID()), lengthof(data.szParentID)-1));
	//data.nFlag |= SOW_FLAG_FOLDER;
	//*pItem = new CSOWFileItem(m_pFolder, m_pFolder->m_pidlFolder, CSOWFileItem::GenerateITEMID(data), TRUE);
	//if (*pItem)
	//	return S_OK;
	//else
	//	return E_FAIL;
	return CNseBaseItem::GetChild(pwstrName, ParseType, pItem);
}

HRESULT CSOWFileItem::GetChildDirect(LPCWSTR pstrFullName, CNseItem** pItem)
{
	WCHAR szReceive[1024]={0};
	WCHAR szBuffer[1024]={0};
	WCHAR szBuffer1[512]={0};
	WCHAR* pszField[50] = {0};
	WCHAR* pszRecord[2] = {0};
	int pnField[50];
	int pnRecord[2];
	int nFieldSize, nRecordSize;
	int nRet = g_Msg.GetFolderByFullPath(pstrFullName, szReceive, lengthof(szReceive));
	if (nRet == 0)
	{
		if (SplitString(szReceive, RECORD_DELIM, 2, &nRecordSize, pszRecord, pnRecord))
		{
			if (nRecordSize == 0)
				return E_FAIL;
			memset(szBuffer, 0, sizeof(szBuffer));
			wcsncpy_s(szBuffer, pszRecord[0], min(pnRecord[0],lengthof(szBuffer)-1));

			if (SplitString(szBuffer, FIELD_DELIM, 50, &nFieldSize, pszField, pnField))
			{
				if (nFieldSize == 0)
					return E_FAIL;
				memset(szBuffer1, 0, sizeof(szBuffer1));
				wcsncpy_s(szBuffer1, pszField[0], min(pnField[0],lengthof(szBuffer1)-1));		// result
				if (wcscmp(szBuffer1, L"1") == 0)
				{
					if (nFieldSize < 10)
						return E_FAIL;
					SOWITEMPIDLINFO data = { 0 };
					wcsncpy_s(data.szID, pszField[1], min(pnField[1],lengthof(data.szID)-1));		// ID
					wcsncpy_s(data.szName, pszField[2], min(pnField[2],lengthof(data.szID)-1));		// Name
					wcsncpy_s(data.szProjectID, pszField[4], min(pnField[4],lengthof(data.szProjectID)-1));	// Project ID
					wcsncpy_s(data.szParentID, pszField[3], min(pnField[3], lengthof(data.szParentID)-1));
					data.nFlag |= SOW_FLAG_FOLDER;
					memset(szBuffer1, 0, sizeof(szBuffer1));
					wcsncpy_s(szBuffer1, pszField[8], min(pnField[8],lengthof(szBuffer1)-1));		// folder type
					if (wcscmp(szBuffer1, L"0") != 0)
						data.nFlag |= SOW_FLAG_SYSFOLDER;
					memset(szBuffer1, 0, sizeof(szBuffer1));
					wcsncpy_s(szBuffer1, pszField[9], min(pnField[9],lengthof(szBuffer1)-1));		// readonly
					if (wcscmp(szBuffer1, L"1") == 0)
						data.nFlag |= SOW_FLAG_READONLY;
					else
						data.nFlag &= ~SOW_FLAG_READONLY;
					*pItem = new CSOWFileItem(m_pFolder, m_pFolder->m_pidlFolder, CSOWFileItem::GenerateITEMID(data), TRUE);
					return S_OK;
				}		
			}
		}
	}
	return E_FAIL;
}

/**
 * Retrieve the list of children of the current folder item.
 */
HRESULT CSOWFileItem::EnumChildren(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aItems)
{
    HRESULT hr = S_OK;
	if (hwndOwner != NULL)
		g_Updater.AddItemForUpdate(hwndOwner);

	// query the folder contents from server...
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

	if (IsRoot())
	{
		if (!g_Msg.IsConnected())
		{
			// 如果Client不响应消息则重新启动
			HKEY hKey; 
			DWORD dwType = REG_SZ; 
			DWORD dwSize; 
			wchar_t data[MAX_PATH]; 
			bool success = false;
			DWORD dwRet = ::RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\SOW\\SOW Cloud", &hKey);
			if (dwRet == ERROR_SUCCESS)
			{
				if (::RegQueryValueEx(hKey,L"Path", NULL, &dwType, (LPBYTE)data, &dwSize) == ERROR_SUCCESS)
					success = true;
				RegCloseKey(hKey);
			}
			if (!success)
			{
				ATLTRACE(L"Cannot find the sow client program!'\n");
				return S_OK;
			}

			SHELLEXECUTEINFO sei = { 0 };
			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT; 
			sei.hwnd = hwndOwner;
			sei.lpVerb = L"open"; 
			sei.lpFile = data;//L"D:\\Projects\\QCP\\trunk\\Source\\SOWClient\\bin\\Debug\\SOWClient.exe";
			sei.nShow = SW_SHOWNORMAL;
			if (::ShellExecuteEx(&sei)) 
			{
				 ::CloseHandle(sei.hProcess);
			}

			// 弹一个模态对话框让用户在此等待；当Client启动并登录成功后，把模态对话框关闭
			TASKDIALOGCONFIG tdc = { 0 };
			tdc.cbSize = sizeof(TASKDIALOGCONFIG);
			tdc.hInstance = _pModule->GetResourceInstance();
			tdc.hwndParent = hwndOwner;
			tdc.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ENABLE_HYPERLINKS;
			tdc.pszWindowTitle = MAKEINTRESOURCE(IDS_NSE_DISPLAYNAME);
			tdc.pszMainIcon = MAKEINTRESOURCE(IDI_APP);
			tdc.pszMainInstruction = MAKEINTRESOURCE(IDS_AUTHCOMPL_TITLE);
			tdc.pszContent = MAKEINTRESOURCE(IDS_AUTHCOMPL_DESCRIPTION);
			tdc.pszFooter = MAKEINTRESOURCE(IDS_AUTHCOMPL_FOOTER);
			TASKDIALOG_BUTTON aButtons[] = 
			{
			  { 200, MAKEINTRESOURCE(IDS_AUTHCOMPL_DONE) },
			  { 201, MAKEINTRESOURCE(IDS_AUTHCOMPL_CANCEL) },
			};
			tdc.cButtons = lengthof(aButtons);
			tdc.pButtons = aButtons;
			int iButton = 0;
			::TaskDialogIndirect(&tdc, &iButton, NULL, NULL);
			if (iButton != 200)
				return S_OK;
		}
		nRet = g_Msg.GetProjects(szReceive, lengthof(szReceive));
		if (nRet == 0)
		{
			if (SplitString(szReceive, RECORD_DELIM, 5000, &nRecordSize, pszRecord, pnRecord))
			{
				if (nRecordSize == 0)
					return hr;
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
							if (nFieldSize < 2)
								continue;
				            SOWITEMPIDLINFO data = { 0 };
							wcsncpy_s(data.szProjectID, pszField[0], min(pnField[0],lengthof(data.szProjectID)-1));	// Project ID
							wcsncpy_s(data.szName, pszField[1], min(pnField[1],lengthof(data.szName)-1));			// Project Name
							wcsncpy_s(data.szID, data.szProjectID, lengthof(data.szProjectID));						// ID
							data.nFlag |= SOW_FLAG_FOLDER|SOW_FLAG_READONLY|SOW_FLAG_SYSFOLDER;
							aItems.Add(new CSOWFileItem(m_pFolder, m_pFolder->m_pidlFolder, CSOWFileItem::GenerateITEMID(data), TRUE));
						}
					}
				}
				else
				{
					{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"Get root folder, ret=%d,err msg=%s\n", nRet, szRetMsg); OutputDebugString(szOutput);}
				}
			}
			else
			{
				{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"SplitString failed\n"); OutputDebugString(szOutput);}
			}
		}
	}
	else
	{
		CCoTaskString strFolder;
		::SHGetNameFromIDList(m_pFolder->m_pidlMonitor, SIGDN_DESKTOPABSOLUTEEDITING, &strFolder);

		nRet = g_Msg.GetFolderContent(GetProjectID(), GetID(), StripFistPathName(StripFistPathName(strFolder)), szReceive, lengthof(szReceive));
		{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"Get none root folder, project=%s, folder=%s, ret=%d, result size=%d\n", GetProjectID(), GetID(), nRet, wcslen(szReceive)); OutputDebugString(szOutput);}
		if (nRet == 0)
		{
			if (SplitString(szReceive, RECORD_DELIM, 5000, &nRecordSize, pszRecord, pnRecord))
			{
				if (nRecordSize == 0)
					return hr;
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
							if (wcscmp(szBuffer1, L"Folder") == 0 && IsBitSet(grfFlags, SHCONTF_FOLDERS))
							{
								if (nFieldSize < 10)
									continue;
								SOWITEMPIDLINFO data = { 0 };
								wcsncpy_s(data.szID, pszField[1], min(pnField[1],lengthof(data.szID)-1));		// ID
								wcsncpy_s(data.szName, pszField[2], min(pnField[2],lengthof(data.szID)-1));		// Name
								wcsncpy_s(data.szProjectID, pszField[4], min(pnField[4],lengthof(data.szProjectID)-1));	// Project ID
								wcsncpy_s(data.szParentID, GetID(), min(wcslen(GetID()), lengthof(data.szParentID)-1));
								data.nFlag |= SOW_FLAG_FOLDER;
								memset(szBuffer1, 0, sizeof(szBuffer1));
								wcsncpy_s(szBuffer1, pszField[8], min(pnField[8],lengthof(szBuffer1)-1));		// folder type
								if (wcscmp(szBuffer1, L"0") != 0)
									data.nFlag |= SOW_FLAG_SYSFOLDER;
								memset(szBuffer1, 0, sizeof(szBuffer1));
								wcsncpy_s(szBuffer1, pszField[9], min(pnField[9],lengthof(szBuffer1)-1));		// readonly
								if (wcscmp(szBuffer1, L"1") == 0)
									data.nFlag |= SOW_FLAG_READONLY;
								else
									data.nFlag &= ~SOW_FLAG_READONLY;
								aItems.Add(new CSOWFileItem(m_pFolder, m_pFolder->m_pidlFolder, CSOWFileItem::GenerateITEMID(data), TRUE));
							}
							else if (wcscmp(szBuffer1, L"File") == 0 && IsBitSet(grfFlags, SHCONTF_NONFOLDERS))
							{
								if (nFieldSize < 11)
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
								wcsncpy_s(data.szProjectID, GetProjectID(), min(wcslen(GetProjectID()), lengthof(data.szProjectID)-1));
								wcsncpy_s(data.szParentID, GetID(), min(wcslen(GetID()), lengthof(data.szParentID)-1));
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
								aItems.Add(new CSOWFileItem(m_pFolder, m_pFolder->m_pidlFolder, CSOWFileItem::GenerateITEMID(data), TRUE));
							}
						}
					}
				}
				else
				{
					{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"Get none root folder, ret=%d,err msg=%s\n", nRet, szRetMsg); OutputDebugString(szOutput);}
				}
			}
			else
			{
				{WCHAR szOutput[256]; wnsprintf(szOutput, lengthof(szOutput)-1, L"SplitString failed\n"); OutputDebugString(szOutput);}
			}
		}
	}
	return S_OK;
}

/**
 * Produce a file-stream instance.
 */
HRESULT CSOWFileItem::GetStream(const VFS_STREAM_REASON& Reason, CNseFileStream** ppFile)
{
	*ppFile = new CSOWFileStream(Reason, this, m_pFolder);
	return *ppFile != NULL ? S_OK : E_OUTOFMEMORY;
}

/**
 * Create a new directory.
 */
HRESULT CSOWFileItem::CreateFolder()
{
	WCHAR szReceive[256]={0};
	int nRet = g_Msg.CreateFolder(GetParentID(), GetName(), szReceive, lengthof(szReceive));
	if (nRet == 0 && GetResult(szReceive) == 0)
		return S_OK;
	return E_FAIL;
}

/**
 * Rename this item.
 */
HRESULT CSOWFileItem::Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName)
{
	WCHAR szReceive[256]={0};
	WCHAR szNewName[100]={0};
	int nRet;
	if (_IsFolder())
	{
		nRet = g_Msg.ChangeFolderName(GetID(), pstrNewName, szReceive, lengthof(szReceive));
		if (nRet == 0 && GetResult(szReceive, szNewName, lengthof(szNewName)) == 0)
		{
			wcscpy(pstrOutputName, szNewName);
		}
	}
	else
	{
		nRet = g_Msg.ChangeFileName(GetID(), pstrNewName, szReceive, lengthof(szReceive));
		if (nRet == 0 && GetResult(szReceive, szNewName, lengthof(szNewName)) == 0)
		{
			wcscpy(pstrOutputName, szNewName);
		}
	}

	if (nRet == 0 && GetResult(szReceive) == 0)
		return S_OK;
	return E_FAIL;
}

/**
 * Delete this item.
 */
HRESULT CSOWFileItem::Delete()
{
	WCHAR szReceive[256]={0};
	int nRet = g_Msg.DeleteFile(GetID(), szReceive, lengthof(szReceive));
	if (nRet == 0 && GetResult(szReceive) == 0)
		return S_OK;
	return E_FAIL;
}

/**
 * Returns the menu-items for an item.
 */
HMENU CSOWFileItem::GetMenu()
{
   UINT uMenuRes = IDM_FILE;
   if (IsFolder()) uMenuRes = IDM_FOLDER;
   if (!IsFolder() || !ReadOnly())
	   return ::LoadMenu(_pModule->GetResourceInstance(), MAKEINTRESOURCE(uMenuRes));
   return NULL;
}

/**
 * Execute a menucommand.
 */
HRESULT CSOWFileItem::ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd)
{
   switch(Cmd.wMenuID) 
   {
   case ID_FILE_OPEN:        return OpenFile(Cmd.hWnd, m_pFolder, GetITEMID());
   case DFM_CMD_PASTE:       return _DoPasteFiles(Cmd);
   case DFM_CMD_NEWFOLDER:   return _DoNewFolder(Cmd, IDS_NEWFOLDER);
   case ID_FILE_NEWFOLDER:   return _DoNewFolder(Cmd, IDS_NEWFOLDER);
   //case ID_FILE_COMMIT:		 return SaveFile(Cmd.hWnd, m_pFolder, GetITEMID());
   //case ID_FILE_PROPERTIES:  return _DoShowProperties(Cmd);
   case ID_FILE_SHARE:		 return ShareFile(Cmd.hWnd, GetID());
   case ID_FILE_UNSHARE:	 return UnshareFile(Cmd.hWnd, GetID());
   case ID_FILE_CHECKOUT:	 return CheckoutFile(Cmd.hWnd, GetID());
   case ID_FILE_CHECKIN:	 return CheckinFile(Cmd.hWnd, GetID());
   //case ID_FILE_ROLLBACK:	 return RollbackFile(Cmd.hWnd, m_pFolder, GetITEMID());
   }
   return E_NOTIMPL;
}


/**
 * Enable or disable menu-items based on the current state of the NSE Item.
 */
HRESULT CSOWFileItem::SetMenuState(const VFS_MENUSTATE& State)
{
	DWORD nCount = 0;
	if (State.pShellItems)
		State.pShellItems->GetCount(&nCount);
	if (nCount == 0)
	{
		::EnableMenuItem(State.hMenu, ID_FILE_COMMIT, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_ROLLBACK, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_SHARE, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_UNSHARE, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_CHECKOUT, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_CHECKIN, MF_DISABLED|MF_GRAYED);
	}
	else if (nCount > 1)
	{
		::EnableMenuItem(State.hMenu, ID_FILE_OPEN, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_COMMIT, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_ROLLBACK, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_SHARE, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_UNSHARE, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_CHECKOUT, MF_DISABLED|MF_GRAYED);
		::EnableMenuItem(State.hMenu, ID_FILE_CHECKIN, MF_DISABLED|MF_GRAYED);
	}
	else
	{
		::EnableMenuItem(State.hMenu, ID_FILE_OPEN, MF_ENABLED);
		CComPtr<IShellItem> spShellItem;
		HR(State.pShellItems->GetItemAt(0, &spShellItem));
		CSOWFileItem *pItem = (CSOWFileItem*)m_pFolder->GenerateChildItemFromShellItem(spShellItem);
		if (pItem->IsFolder()) 
		{
			if (ReadOnly())
			{
				::EnableMenuItem(State.hMenu, ID_FILE_NEWFOLDER, MF_DISABLED|MF_GRAYED);
			}
		}
		else
		{
			//if (pItem->Changed())
			//{
			//	::EnableMenuItem(State.hMenu, ID_FILE_COMMIT, MF_ENABLED);
			//	::EnableMenuItem(State.hMenu, ID_FILE_ROLLBACK, MF_ENABLED);
			//}
			//else
			//{
			//	::EnableMenuItem(State.hMenu, ID_FILE_COMMIT, MF_DISABLED|MF_GRAYED);
			//	::EnableMenuItem(State.hMenu, ID_FILE_ROLLBACK, MF_DISABLED|MF_GRAYED);
			//}
			bool bShare = !pItem->AllowShare() && (!pItem->HasOwner() || !pItem->ReadOnly());
			bool bUnshare = pItem->AllowShare() && (!pItem->HasOwner() || !pItem->ReadOnly());
			bool bCheckout = pItem->AllowShare() && !pItem->HasOwner();
			bool bCheckin = pItem->AllowShare() && !pItem->ReadOnly();
			::EnableMenuItem(State.hMenu, ID_FILE_SHARE, bShare ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
			::EnableMenuItem(State.hMenu, ID_FILE_UNSHARE, bUnshare ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
			::EnableMenuItem(State.hMenu, ID_FILE_CHECKOUT, bCheckout ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
			::EnableMenuItem(State.hMenu, ID_FILE_CHECKIN, bCheckin ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
		}
	}
	return S_OK;
}

HRESULT CSOWFileItem::IsDropDataAvailable(IDataObject* pDataObj)
{
   // We support file drops
   return (!ReadOnly() && DataObj_HasFileClipFormat(pDataObj)) ? S_OK : S_FALSE;
}


// Implementation

LPCWSTR CSOWFileItem::GetName()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return m_pData->szName;
}

LPCWSTR CSOWFileItem::GetProjectID()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return m_pData->szProjectID;
}

LPCWSTR CSOWFileItem::GetParentID()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return m_pData->szParentID;
}

LPCWSTR CSOWFileItem::GetMD5()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return m_pData->szMD5;
}

LPCWSTR CSOWFileItem::GetID()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return m_pData->szID;
}

LPCWSTR CSOWFileItem::GetOwner()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return m_pData->szOwner;
}

BOOL CSOWFileItem::_IsFolder()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return FALSE;
	return (m_pData->nFlag & SOW_FLAG_FOLDER);
}

BOOL CSOWFileItem::ReadOnly()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return NULL;
	return (m_pData->nFlag & SOW_FLAG_READONLY);
}

BOOL CSOWFileItem::Changed()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return FALSE;
	return (m_pData->nFlag & SOW_FLAG_CHANGED);
}

BOOL CSOWFileItem::SysFolder()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return FALSE;
	return (m_pData->nFlag & SOW_FLAG_SYSFOLDER);
}

BOOL CSOWFileItem::AllowShare()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return FALSE;
	return (m_pData->nFlag & SOW_FLAG_ALLOWSHARE);
}

BOOL CSOWFileItem::HasOwner()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return FALSE;
	return (m_pData->nFlag & SOW_FLAG_HASOWNER);
}

LONG CSOWFileItem::GetSize()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return 0;
	return m_pData->nSize;
}

DWORD CSOWFileItem::GetFlag()
{
	if (m_pData == NULL || m_pData->magic != SOW_MAGICID)
		return 0;
	return m_pData->nFlag;
}

typedef struct tagSOWLAUNCHDATA
{
   HWND hWnd;
   HWND hOwnerWnd;
   WCHAR szVerb[20];
   WCHAR szProjectID[40];
   WCHAR szFileID[40];
   WCHAR szNewName[MAX_PATH];
   CPidl pidlPath;
   CPidl pidlFile;
   BOOL bLaunch;
   DWORD nFlag;
} SOWLAUNCHDATA;


DWORD WINAPI SOWLaunchThread(LPVOID pData)
{
   SOWLAUNCHDATA data = * (SOWLAUNCHDATA*) pData;
   delete (SOWLAUNCHDATA*) pData;

   ATLTRACE(L"LaunchThread  filename='%s'\n", data.szNewName);

   bool bLaunch = true;

   // Check whether the file need to be downloaded or updated
   WCHAR szReceive[512] = {0};
   WCHAR szFile[512] = {0};
   WCHAR szPath[512] = {0};
   BOOL bUpdated;
   int nRet = g_Msg.CheckFile(data.szProjectID, data.szFileID, data.bLaunch ? L"1" : L"2", szReceive, lengthof(szReceive), szFile, lengthof(szFile), &bUpdated);
   if (nRet != 0)
   {
	   ATLTRACE(L"LaunchThread - Failed to get work copy\n");
	   return -1;
   }
   GetPathName(szFile, szPath, lengthof(szPath));
   // Copy file from virtual drive to disk...
   if (!bUpdated)
   {
	  CComPtr<IFileOperation> spFO;
	  HRESULT Hr = ::SHCreateFileOperation(data.hWnd, FOF_NOCOPYSECURITYATTRIBS | FOFX_NOSKIPJUNCTIONS | FOF_NOCONFIRMATION | FOFX_DONTDISPLAYDESTPATH, &spFO);
	  if (FAILED(Hr)) return 0;
	  CComPtr<IShellItem> spSourceFile;
	  CComPtr<IShellItem> spTargetFile;
	  ::SHCreateItemFromIDList(data.pidlFile, IID_PPV_ARGS(&spSourceFile));
	  ::SHCreateItemFromParsingName(szPath, NULL, IID_PPV_ARGS(&spTargetFile));
	  spFO->CopyItem(spSourceFile, spTargetFile, data.szNewName, NULL);
	  Hr = spFO->PerformOperations();
	  if (FAILED(Hr)) bLaunch = false;
	  memset(szReceive, 0, sizeof(szReceive));
	  g_Msg.SetFileVersion(data.szProjectID, data.szFileID, szReceive, lengthof(szReceive));
   }
   if (!data.bLaunch)
	   return 0;

   // Check security first. Use Windows AttachmentServices to determine
   // and display warning prompt.
   if (bLaunch && _ShellModule.GetConfigBool(VFS_CAN_ATTACHMENTSERVICES))
   {
      LPCTSTR pstrExt = ::PathFindExtension(data.szNewName);
      BOOL bIsDangerous = ::AssocIsDangerous(pstrExt);
      if (bIsDangerous) 
	  {
         CComPtr<IAttachmentExecute> spExecute;
         spExecute.CoCreateInstance(CLSID_AttachmentServices);
         if (spExecute != NULL) 
		 {
            CComBSTR bstrTitle, bstrVendor;
            bstrVendor.LoadString(IDS_NSE_VENDOR);
            bstrTitle.LoadString(IDS_NSE_DISPLAYNAME);
            spExecute->SetSource(bstrVendor);
            spExecute->SetClientTitle(bstrTitle);
            spExecute->SetClientGuid(CLSID_ShellFolder);
            if (SUCCEEDED(spExecute->SetLocalPath(szFile))) 
			{               
               ATTACHMENT_ACTION ac = ATTACHMENT_ACTION_CANCEL;
               spExecute->Prompt(data.hWnd, ATTACHMENT_PROMPT_EXEC, &ac);
               bLaunch = (ac == ATTACHMENT_ACTION_EXEC);
            }
         }
      }
   }

   // Launch the file now?
   if (bLaunch)
   {
	  // if shared, check out
      if (data.nFlag & SOW_FLAG_ALLOWSHARE)
	  {
		   WCHAR szReceive[512] = {0};
		   WCHAR szRetMsg[512] = {0};
		   int nRet = g_Msg.Checkout(data.szFileID, szReceive, lengthof(szReceive));
		   if (nRet != 0)
		   {
			   CComBSTR bstrCaption;
			   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
			   CComBSTR bstrText;
			   bstrText.LoadString(IDS_SERVER_ERR);
			   MessageBox(data.hWnd, bstrText, bstrCaption, MB_OK);
			   return -1;
		   }
		   else
		   {
			   GetCommandResult(szReceive, &nRet, szRetMsg, lengthof(szRetMsg));
			   if (nRet != 0)
			   {
				   CComBSTR bstrCaption;
				   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
				   CComBSTR bstrText;
				   bstrText.LoadString(IDS_SERVER_ERR);
				   if (wcslen(szRetMsg) > 0)
					   bstrText += szRetMsg;
				   MessageBox(data.hWnd, bstrText, bstrCaption, MB_OK);
				   return -1;
			   }
		   }
	  }
	  // add to update list
	  g_Updater.AddItemForUpdate(data.hOwnerWnd);

      SHELLEXECUTEINFO sei = { 0 };
      sei.cbSize = sizeof(SHELLEXECUTEINFO);
      sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT; 
      sei.hwnd = data.hWnd;
      sei.lpVerb = data.szVerb; 
      sei.lpFile = szFile;
      sei.nShow = SW_SHOWNORMAL;
      if (::ShellExecuteEx(&sei)) 
	  {
         // FIX: Post empty message to parent to speed up destruction of Progress dialog
         if (data.hWnd != NULL && ::IsWindow(data.hWnd)) ::PostMessage(data.hWnd, WM_NULL, 0, 0L);
         ::CloseHandle(sei.hProcess);
      }
   }

   ATLTRACE(L"LaunchThread - Done\n");

   return 0;
};

BOOL CSOWFileItem::OpenFile(HWND hWnd, CShellFolder* pFolder, PCUITEMID_CHILD pidlItem, LPCTSTR pstrVerb)
{
	SOWLAUNCHDATA* pData = new SOWLAUNCHDATA;
	pData->hWnd = hWnd;
	pData->hOwnerWnd = pFolder->m_hwndOwner;
	_tcscpy_s(pData->szVerb, lengthof(pData->szVerb), pstrVerb);
	_tcscpy_s(pData->szProjectID, lengthof(pData->szProjectID), GetProjectID());
	_tcscpy_s(pData->szFileID, lengthof(pData->szFileID), GetID());
	_tcscpy_s(pData->szNewName, lengthof(pData->szNewName), GetName());
	pData->pidlFile = pFolder->m_pidlMonitor + pidlItem;
	pData->pidlPath = pFolder->m_pidlMonitor;
	pData->bLaunch = TRUE;
	pData->nFlag = GetFlag();
	if (!::SHCreateThread(SOWLaunchThread, pData, CTF_COINIT | CTF_PROCESS_REF, NULL)) return E_FAIL;

	return S_OK;
}

BOOL CSOWFileItem::RollbackFile(HWND hWnd, CShellFolder* pFolder, PCUITEMID_CHILD pidlItem)
{
	SOWLAUNCHDATA* pData = new SOWLAUNCHDATA;
	pData->hWnd = hWnd;
	_tcscpy_s(pData->szProjectID, lengthof(pData->szProjectID), GetProjectID());
	_tcscpy_s(pData->szFileID, lengthof(pData->szFileID), GetID());
	_tcscpy_s(pData->szNewName, lengthof(pData->szNewName), GetName());
	pData->pidlFile = pFolder->m_pidlMonitor + pidlItem;
	pData->pidlPath = pFolder->m_pidlMonitor;
	pData->bLaunch = FALSE;
	pData->nFlag = GetFlag();
	if (!::SHCreateThread(SOWLaunchThread, pData, CTF_COINIT | CTF_PROCESS_REF, NULL)) return E_FAIL;

	return S_OK;
}

DWORD WINAPI SOWSaveThread(LPVOID pData)
{
   SOWLAUNCHDATA data = * (SOWLAUNCHDATA*) pData;
   delete (SOWLAUNCHDATA*) pData;

   ATLTRACE(L"SaveThread  filename='%s'\n", data.szNewName);

   // Get the local work copy file
   WCHAR szReceive[512] = {0};
   WCHAR szFile[512] = {0};
   WCHAR szPath[512] = {0};
   BOOL bUpdated, bModified, bIllegal;
   int nRet = g_Msg.CheckFile(data.szProjectID, data.szFileID, L"0", szReceive, lengthof(szReceive), szFile, lengthof(szFile), &bUpdated, &bModified, &bIllegal);
   if (nRet != 0)
   {
	   ATLTRACE(L"SaveThread - Failed to get work copy\n");
	   return -1;
   }
   if (bModified && bIllegal)
   {
	   CComBSTR bstrCaption;
	   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
	   CComBSTR bstrText;
	   bstrText.LoadString(IDS_ILLEGAL);
	   MessageBox(data.hWnd, bstrText, bstrCaption, MB_OK);
	   return -1;
   }
   if (bUpdated && !bModified)
   {
	   CComBSTR bstrCaption;
	   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
	   CComBSTR bstrText;
	   bstrText.LoadString(IDS_UPDATED);
	   MessageBox(data.hWnd, bstrText, bstrCaption, MB_OK);
	   return -1;
   }
   GetPathName(szFile, szPath, lengthof(szPath));
   // Save file from disk to virtual drive
   if (bModified && !bIllegal)
   {
	  CComPtr<IFileOperation> spFO;
	  HRESULT Hr = ::SHCreateFileOperation(data.hWnd, FOF_NOCOPYSECURITYATTRIBS | FOFX_NOSKIPJUNCTIONS | FOF_NOCONFIRMATION | FOFX_DONTDISPLAYDESTPATH, &spFO);
	  if (FAILED(Hr)) return 0;
	  CComPtr<IShellItem> spSourceFile;
	  CComPtr<IShellItem> spTargetFile;
	  ::SHCreateItemFromIDList(data.pidlPath, IID_PPV_ARGS(&spTargetFile));
	  ::SHCreateItemFromParsingName(szFile, NULL, IID_PPV_ARGS(&spSourceFile));
	  spFO->CopyItem(spSourceFile, spTargetFile, data.szNewName, NULL);
	  Hr = spFO->PerformOperations();
	  if (SUCCEEDED(Hr))
	  {
		  memset(szReceive, 0, sizeof(szReceive));
		  g_Msg.SetFileVersion(data.szProjectID, data.szFileID, szReceive, lengthof(szReceive));
		  ::SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, data.pidlPath);
	  }
   }

   ATLTRACE(L"SaveThread - Done\n");

   return 0;
};


BOOL CSOWFileItem::SaveFile(HWND hWnd, CShellFolder* pFolder, PCUITEMID_CHILD pidlItem)
{
	SOWLAUNCHDATA* pData = new SOWLAUNCHDATA;
	pData->hWnd = hWnd;
	_tcscpy_s(pData->szProjectID, lengthof(pData->szProjectID), GetProjectID());
	_tcscpy_s(pData->szFileID, lengthof(pData->szFileID), GetID());
	_tcscpy_s(pData->szNewName, lengthof(pData->szNewName), GetName());
	pData->pidlFile = pFolder->m_pidlMonitor + pidlItem;
	pData->pidlPath = pFolder->m_pidlMonitor;
	pData->bLaunch = FALSE;
	pData->nFlag = GetFlag();
	if (!::SHCreateThread(SOWSaveThread, pData, CTF_COINIT | CTF_PROCESS_REF, NULL)) return E_FAIL;

	return S_OK;
}

BOOL CSOWFileItem::CheckoutFile(HWND hWnd, LPCWSTR pstrFileID)
{
   WCHAR szReceive[512] = {0};
   int nRet = g_Msg.Checkout(pstrFileID, szReceive, lengthof(szReceive));
   if (nRet != 0)
   {
	   CComBSTR bstrCaption;
	   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
	   CComBSTR bstrText;
	   bstrText.LoadString(IDS_SERVER_ERR);
	   WCHAR szMsg[100]={0};
	   if (GetResult(szReceive, szMsg, lengthof(szMsg)) == 0)
	   {
		   bstrText += L":";
		   bstrText += szMsg;
	   }
	   MessageBox(hWnd, bstrText, bstrCaption, MB_OK);
	   return E_FAIL;
   }
   else
   {
   }
   return S_OK;
}

BOOL CSOWFileItem::CheckinFile(HWND hWnd, LPCWSTR pstrFileID)
{
   WCHAR szReceive[512] = {0};
   int nRet = g_Msg.Checkin(pstrFileID, szReceive, lengthof(szReceive));
   if (nRet != 0)
   {
	   CComBSTR bstrCaption;
	   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
	   CComBSTR bstrText;
	   bstrText.LoadString(IDS_SERVER_ERR);
	   WCHAR szMsg[100]={0};
	   if (GetResult(szReceive, szMsg, lengthof(szMsg)) == 0)
	   {
		   bstrText += L":";
		   bstrText += szMsg;
	   }
	   MessageBox(hWnd, bstrText, bstrCaption, MB_OK);
	   return E_FAIL;
   }
   else
   {
   }
   return S_OK;
}

BOOL CSOWFileItem::ShareFile(HWND hWnd, LPCWSTR pstrFileID)
{
   WCHAR szReceive[512] = {0};
   int nRet = g_Msg.ShareFile(pstrFileID, szReceive, lengthof(szReceive));
   if (nRet != 0)
   {
	   CComBSTR bstrCaption;
	   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
	   CComBSTR bstrText;
	   bstrText.LoadString(IDS_SERVER_ERR);
	   WCHAR szMsg[100]={0};
	   if (GetResult(szReceive, szMsg, lengthof(szMsg)) == 0)
	   {
		   bstrText += L":";
		   bstrText += szMsg;
	   }
	   MessageBox(hWnd, bstrText, bstrCaption, MB_OK);
	   return E_FAIL;
   }
   else
   {
	   //SHChangeNotify(0x8000000, 0x1000, 0, 0);
	   //::SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST|SHCNF_FLUSH, _GetFullPidl(), NULL);
	   
	   // Tell View to update item
	   PIDLIST_RELATIVE pidlTmp = ::ILCloneChild(GetITEMID());
	   SOWITEMPIDLINFO *pData = (SOWITEMPIDLINFO*)pidlTmp;
	   pData->nFlag |= SOW_FLAG_ALLOWSHARE;
	   pData->nFlag |= SOW_FLAG_HASOWNER;
	   PCITEMID_CHILD ppidl[2] = {GetITEMID(), (PCITEMID_CHILD)pidlTmp};
	   if (ShellFolderView_UpdateObject(m_pFolder->m_hwndOwner, ppidl) == -1)
	   {
		   ::ILFree(pidlTmp);
	   }
   }
   return S_OK;
}

BOOL CSOWFileItem::UnshareFile(HWND hWnd, LPCWSTR pstrFileID)
{
   WCHAR szReceive[512] = {0};
   int nRet = g_Msg.UnshareFile(pstrFileID, szReceive, lengthof(szReceive));
   if (nRet != 0)
   {
	   CComBSTR bstrCaption;
	   bstrCaption.LoadString(IDS_NSE_DISPLAYNAME);
	   CComBSTR bstrText;
	   bstrText.LoadString(IDS_SERVER_ERR);
	   WCHAR szMsg[100]={0};
	   if (GetResult(szReceive, szMsg, lengthof(szMsg)) == 0)
	   {
		   bstrText += L":";
		   bstrText += szMsg;
	   }
	   MessageBox(hWnd, bstrText, bstrCaption, MB_OK);
	   return E_FAIL;
   }
   else
   {
	   //::SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST|SHCNF_FLUSH, _GetFullPidl(), NULL);
	   // Tell View to update item
	   PIDLIST_RELATIVE pidlTmp = ::ILCloneChild(GetITEMID());
	   SOWITEMPIDLINFO *pData = (SOWITEMPIDLINFO*)pidlTmp;
	   pData->nFlag &= ~SOW_FLAG_ALLOWSHARE;
	   pData->nFlag &= ~SOW_FLAG_HASOWNER;
	   PCITEMID_CHILD ppidl[2] = {GetITEMID(), (PCITEMID_CHILD)pidlTmp};
	   if (ShellFolderView_UpdateObject(m_pFolder->m_hwndOwner, ppidl) == -1)
	   {
		   ::ILFree(pidlTmp);
	   }
   }
   return S_OK;
}
