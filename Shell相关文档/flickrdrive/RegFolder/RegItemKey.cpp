
#include "stdafx.h"

#include "RegFileSystem.h"
#include "ShellFolder.h"


///////////////////////////////////////////////////////////////////////////////
// CRegItemKey defines

// The top-level keys we wish to display
static LPCWSTR s_aRoots[] = { L"HKEY_CLASSES_ROOT", L"HKEY_CURRENT_USER", L"HKEY_CURRENT_CONFIG", L"HKEY_LOCAL_MACHINE" };

// The "HKEY_" in "HKEY_CURRENT_USER"
#define HKEY_PREFIX_LEN   5


///////////////////////////////////////////////////////////////////////////////
// CRegItemKey

CRegItemKey::CRegItemKey(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) :
   CNseBaseItem(pFolder, pidlFolder, pidlItem, bReleaseItem)
{
   if( pidlItem == NULL ) 
   {
      // Create empty root item
      static REGKEYPIDLINFO s_Info = { 0 };
      s_Info.magic = REG_MAGICID_KEY;
      m_pRegInfo = &s_Info;
   }
   else 
   {
      // Extract item data
      m_pRegInfo = reinterpret_cast<const REGKEYPIDLINFO*>(pidlItem);
   }
}

/**
 * Get the item type.
 */
BYTE CRegItemKey::GetType()
{
   return REG_MAGICID_KEY;   
}

/**
 * Get system icon index.
 * This is slightly faster for the Shell than using GetExtractIcon().
 * Return S_FALSE if no system index exists.
 */
HRESULT CRegItemKey::GetSysIcon(UINT uIconFlags, int* pIconIndex)
{
   // Use our SHGetFileSysIcon() method to get the System Icon index
   return ::SHGetFileSysIcon(m_pRegInfo->cName, FILE_ATTRIBUTE_DIRECTORY, uIconFlags, pIconIndex);
}

/**
 * Create Shell object for extracting icon and image.
 * This will create the Shell object asked for by the Shell to generate
 * the display icon.
 */
HRESULT CRegItemKey::GetExtractIcon(REFIID riid, LPVOID* ppRetVal)
{
   // Use the SHCreateFileExtractIcon() method to create a default Directory icon object
   // for IID_IExtractIcon. We don't support IID_IExtractImage here.
   return ::SHCreateFileExtractIcon(m_pRegInfo->cName, FILE_ATTRIBUTE_DIRECTORY, riid, ppRetVal);
}

/**
 * Get information about column definition.
 * We return details of a column requested, plus information about the 
 * combined set of properties supported by items contained in this folders. We 
 * decide which columns to show by default here too.
 */
HRESULT CRegItemKey::GetColumnInfo(UINT iColumn, VFS_COLUMNINFO& Column)
{
   static VFS_COLUMNINFO aColumns[] = {
      { PKEY_ItemNameDisplay,            SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT,                    0 },
      { PKEY_DateModified,               SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT | SHCOLSTATE_SLOW,  0 },
      { PKEY_ItemPathDisplay,            SHCOLSTATE_TYPE_STR  | SHCOLSTATE_SECONDARYUI,                    0 },
      { PKEY_RegistryType,               SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT,                    0 },
      { PKEY_RegistryValueType,          SHCOLSTATE_TYPE_STR  | SHCOLSTATE_SECONDARYUI,                    0 },
      { PKEY_RegistryValue,              SHCOLSTATE_TYPE_STR  | SHCOLSTATE_SECONDARYUI,                    VFS_COLF_WRITEABLE },
      { PKEY_FileAttributes,             SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ItemType,                   SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ItemTypeText,               SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ParsingPath,                SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },
      { PKEY_ItemPathDisplayNarrow,      SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         0 },      
      { PKEY_SFGAOFlags,                 SHCOLSTATE_TYPE_INT  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_Volume_IsRoot,              SHCOLSTATE_TYPE_INT  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_InfoTip,           SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_TileInfo,          SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_FullDetails,       SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_PreviewTitle,      SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_PreviewDetails,    SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
      { PKEY_PropList_ExtendedTileInfo,  SHCOLSTATE_TYPE_STR  | SHCOLSTATE_HIDDEN,                         VFS_COLF_NOTCOLUMN },
   };
   if( iColumn >= lengthof(aColumns) ) return E_FAIL;
   Column = aColumns[iColumn];
   return S_OK;
}

/**
 * Return item information.
 * We support the properties for the columns as well as a number of
 * administrative information bits (such as what properties to display
 * in the Details panel).
 */
HRESULT CRegItemKey::GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v)
{
   if( pkey == PKEY_ParsingName ) {
      return ::InitPropVariantFromString(m_pRegInfo->cName, &v);
   } 
   if( pkey == PKEY_ItemName ) {
      return ::InitPropVariantFromString(m_pRegInfo->cName, &v);
   }
   // For convenience we only display the name CURRENT_USER (not HKEY_CURRENT_USER) for
   // the root items. This also demonstrates how the parsing name can be different from
   // the display name.
   if( pkey == PKEY_ItemNameDisplay ) {
      if( _IsTopLevel() ) return ::InitPropVariantFromString(m_pRegInfo->cName + HKEY_PREFIX_LEN, &v);
      return ::InitPropVariantFromString(m_pRegInfo->cName, &v);
   }
   // Return our custom Registry properties and standard time-related properties
   // for this item.
   if( pkey == PKEY_RegistryType ) {
      return ::InitPropVariantFromUInt32(0UL, &v);
   }
   if( pkey == PKEY_DateModified ) {
      const WIN32_FIND_DATA wfd = GetFindData();
      return ::InitPropVariantFromFileTime(&wfd.ftLastWriteTime, &v);
   }
   // Returning file-attributes allow keys to be sorted/displayed
   // before values in the list.
   if( pkey == PKEY_FileAttributes ) {
      return ::InitPropVariantFromUInt32(FILE_ATTRIBUTE_DIRECTORY, &v);
   }
   // Return properties for display details. These define what properties to
   // display in various places in the Shell, such as the hover-tip and Details panel.
   if( pkey == PKEY_PropList_TileInfo ) {
      return ::InitPropVariantFromString(L"prop:Windows.Registry.Type;", &v);
   }
   if( pkey == PKEY_PropList_ExtendedTileInfo ) {
      return ::InitPropVariantFromString(L"prop:Windows.Registry.Type;System.DateModified;", &v);
   }
   if( pkey == PKEY_PropList_PreviewTitle ) {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;Windows.Registry.Type;", &v);
   }
   if( pkey == PKEY_PropList_PreviewDetails ) {
      return ::InitPropVariantFromString(L"prop:System.DateModified;", &v);
   }
   if( pkey == PKEY_PropList_InfoTip ) {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;Windows.Registry.Type;", &v);
   }
   if( pkey == PKEY_PropList_FullDetails ) {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;Windows.Registry.Type;System.DateModified;", &v);
   }
   return CNseBaseItem::GetProperty(pkey, v);
}

/**
 * Return SFGAOF flags for this item.
 * These flags tells the Shell about the capabilities of this item. Here we
 * can toggle the ability to copy/delete/rename an item.
 */
SFGAOF CRegItemKey::GetSFGAOF(SFGAOF dwMask)
{
   return SFGAO_FOLDER
          | SFGAO_BROWSABLE
          | SFGAO_CANDELETE 
          | SFGAO_CANRENAME 
          | SFGAO_HASSUBFOLDER;
}

/**
 * Return file information.
 * We use this to return a simple structure with basic information about
 * our item.
 */
VFS_FIND_DATA CRegItemKey::GetFindData()
{
   VFS_FIND_DATA wfd = { 0 };
   wcscpy_s(wfd.cFileName, lengthof(wfd.cFileName), m_pRegInfo->cName);
   wfd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
   // Get the write-timestamp if needed; to do so we must interrogate the Registry
   // key with RegQueryInfoKey().
   wfd.ftLastWriteTime = m_pRegInfo->ftLastWrite;
   if( wfd.ftLastWriteTime.dwLowDateTime == 0 ) {
      CRegKey key;
      if( SUCCEEDED( _OpenRegKey(REGACC_OPEN, KEY_QUERY_VALUE, REGPATH_FULLITEM, NULL, key) ) ) {
         ::RegQueryInfoKey(key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &wfd.ftLastWriteTime);
      }
   }
   return wfd;
}

/**
 * Return the folder settings.
 * The Folder Settings structure contains optional values for the initial
 * display of the folder.
 */
VFS_FOLDERSETTINGS CRegItemKey::GetFolderSettings()
{
   VFS_FOLDERSETTINGS Settings = { 0 };
   // Control default view-mode for the 1st, 2nd and other tree levels.
   if( ::ILIsEmpty(m_pidlFolder) ) Settings.ViewMode = FLVM_ICONS, Settings.cxyIcon = 96;
   else if( ILIsChild(m_pidlFolder) ) Settings.ViewMode = FLVM_ICONS;
   else Settings.ViewMode = FLVM_DETAILS;
   return Settings;
}

/**
 * Return the menu for the item.
 */
HMENU CRegItemKey::GetMenu()
{
   return ::LoadMenu(_pModule->GetResourceInstance(), MAKEINTRESOURCE(IDM_FOLDER));
}

/**
 * Execute a menu command.
 */
HRESULT CRegItemKey::ExecuteMenuCommand(VFS_MENUCOMMAND& Cmd)
{
   switch( Cmd.wMenuID ) {
   case DFM_CMD_NEWFOLDER:       return _DoNewKey(Cmd, IDS_UNNAMED);
   case ID_NEW_KEY:              return _DoNewKey(Cmd, IDS_UNNAMED);
   case ID_NEW_VALUE_DEFAULT:    return _DoNewValue(Cmd, IDS_EMPTY, REG_SZ);
   case ID_NEW_VALUE_SZ:         return _DoNewValue(Cmd, IDS_UNNAMED, REG_SZ);
   case ID_NEW_VALUE_DWORD:      return _DoNewValue(Cmd, IDS_UNNAMED, REG_DWORD);
   case ID_NEW_VALUE_QWORD:      return _DoNewValue(Cmd, IDS_UNNAMED, REG_QWORD);      
   case ID_NEW_VALUE_EXPAND_SZ:  return _DoNewValue(Cmd, IDS_UNNAMED, REG_EXPAND_SZ);
   }
   return E_NOTIMPL;
}

/**
 * Create an NSE Item instance from a child PIDL.
 */
CNseItem* CRegItemKey::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem)
{
   // Use a pidl-wrapper to validate the correctness of the PIDL and spawn
   // an NSE Item based on the type.
   CPidlMemPtr<REGKEYPIDLINFO> pItem = pidlItem;
   if( pItem.IsType(REG_MAGICID_KEY) ) return new CRegItemKey(pFolder, pidlFolder, pidlItem, bReleaseItem);
   if( pItem.IsType(REG_MAGICID_VALUE) ) return new CRegItemValue(pFolder, pidlFolder, pidlItem, bReleaseItem);
   return NULL;
}

/**
 * Create an NSE Item from static data.
 */
CNseItem* CRegItemKey::GenerateChild(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, const WIN32_FIND_DATA wfd)
{
   if( IsBitSet(wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ) {
      return new CRegItemKey(pFolder, pidlFolder, CRegItemKey::GenerateITEMID(wfd), TRUE);
   }
   return new CRegItemValue(pFolder, pidlFolder, CRegItemValue::GenerateITEMID(wfd), TRUE);
}

/**
 * Look up a single child item (Registry key or value) by name.
 */
HRESULT CRegItemKey::GetChild(LPCWSTR pwstrName, SHGNO ParseType, CNseItem** pItem)
{
   REGKEYPIDLINFO data = { 0 };
   if( IsRoot() ) 
   {
      // Find it in the "HKEY_CURRENT_USER", etc. keys of the root.
      // If it's the display-name then look for "CURRENT_USER" too.
      for( int i = 0; i < lengthof(s_aRoots); i++ ) {
         if( (_wcsicmp(pwstrName, s_aRoots[i]) == 0)
             || (ParseType == SHGDN_FOREDITING && _wcsicmp(pwstrName, s_aRoots[i] + HKEY_PREFIX_LEN) == 0) ) 
         {
            wcscpy_s(data.cName, lengthof(data.cName), s_aRoots[i]);
            *pItem = new CRegItemKey(m_pFolder, m_pidlFolder, CRegItemKey::GenerateITEMID(data), TRUE);
            return *pItem != NULL ? S_OK : E_OUTOFMEMORY;
         }
      }
   }
   else
   {
      CRegKey key;
      HR( _OpenRegKey(REGACC_OPEN, KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, REGPATH_FULLITEM, NULL, key) );
      wcscpy_s(data.cName, lengthof(data.cName), pwstrName);
      // Hotfix of (default) item which is internally known as '\0'
      CComBSTR bstrDefault;
      bstrDefault.LoadString(IDS_REGVAL_DEFAULT);
      if( bstrDefault == data.cName ) data.cName[0] = '\0';
      if( data.cName[0] == '\0' ) pwstrName = NULL;
      // Check if it is a value...
      if( ::RegQueryValueEx(key, pwstrName, NULL, &data.dwValueType, NULL, NULL) == ERROR_SUCCESS ) {
         *pItem = new CRegItemValue(m_pFolder, m_pidlFolder, CRegItemValue::GenerateITEMID(data), TRUE);
         return *pItem != NULL ? S_OK : E_OUTOFMEMORY;
      }
      // Check if it is a child key...
      CRegKey keyChild;
      if( key.Open(key, pwstrName, KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE) == ERROR_SUCCESS ) {
         *pItem = new CRegItemKey(m_pFolder, m_pidlFolder, CRegItemKey::GenerateITEMID(data), TRUE);
         return *pItem != NULL ? S_OK : E_OUTOFMEMORY;
      }
   }
   return E_FAIL;
}

/**
 * Retrieve the list of children of the current folder item.
 */
HRESULT CRegItemKey::EnumChildren(HWND hwndOwner, SHCONTF grfFlags, CSimpleValArray<CNseItem*>& aItems)
{
   if( IsRoot() )
   {
      if( IsBitSet(grfFlags, SHCONTF_FOLDERS) )
      {
         for( int i = 0; i < lengthof(s_aRoots); i++ ) {
            REGKEYPIDLINFO data = { 0 };
            wcscpy_s(data.cName, lengthof(data.cName), s_aRoots[i]); 
            aItems.Add( new CRegItemKey(m_pFolder, m_pFolder->m_pidlFolder, CRegItemKey::GenerateITEMID(data), TRUE) );
         }
      }
   }
   else
   {
      CRegKey key;
      HRESULT Hr = _OpenRegKey(REGACC_OPEN, KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, REGPATH_FULLITEM, NULL, key);
      if( FAILED(Hr) ) return S_OK;
      // Enumerate all sub-keys...
      DWORD nSubKeys = 0, nValues = 0;
      ::RegQueryInfoKey(key, NULL, NULL, NULL, &nSubKeys, NULL, NULL, &nValues, NULL, NULL, NULL, NULL);
      if( IsBitSet(grfFlags, SHCONTF_FOLDERS) )
      {
         for( DWORD dwIndex = 0; dwIndex < nSubKeys; dwIndex++ ) {
            REGKEYPIDLINFO data = { 0 };
            DWORD cchName = lengthof(data.cName);
            if( ::RegEnumKeyEx(key, dwIndex, data.cName, &cchName, NULL, NULL, NULL, &data.ftLastWrite) != ERROR_SUCCESS ) continue;
            aItems.Add( new CRegItemKey(m_pFolder, m_pFolder->m_pidlFolder, CRegItemKey::GenerateITEMID(data), TRUE) );
         }
      }
      // Enumerate all values...
      if( IsBitSet(grfFlags, SHCONTF_NONFOLDERS) )
      {
         for( DWORD dwIndex = 0; dwIndex < nValues; dwIndex++ ) {
            REGKEYPIDLINFO data = { 0 };
            DWORD cchName = lengthof(data.cName);
            if( ::RegEnumValue(key, dwIndex, data.cName, &cchName, NULL, &data.dwValueType, NULL, NULL) != ERROR_SUCCESS ) continue;
            aItems.Add( new CRegItemValue(m_pFolder, m_pFolder->m_pidlFolder, CRegItemValue::GenerateITEMID(data), TRUE) );
         }
      }
   }
   return S_OK;
}

/**
 * Create a new sub-key.
 */
HRESULT CRegItemKey::CreateFolder()
{
   // Create this item as a new Registry key
   CRegKey key;
   HR( _OpenRegKey(REGACC_CREATE, KEY_ALL_ACCESS, REGPATH_FULLITEM, NULL, key) );
   return S_OK;
}

/**
 * Rename this item.
 */
HRESULT CRegItemKey::Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName)
{
   CRegKey keySrc, keyDest, keyParent;
   HR( _OpenRegKey(REGACC_OPEN, KEY_READ, REGPATH_FULLITEM, NULL, keySrc) );
   HR( _OpenRegKey(REGACC_OPEN, KEY_ALL_ACCESS, REGPATH_PARENT, NULL, keyParent) );
   HR( _OpenRegKey(REGACC_CREATE, KEY_WRITE, REGPATH_PARENT, pstrNewName, keyDest) );
   HR( _OpenRegKey(REGACC_OPEN, KEY_ALL_ACCESS, REGPATH_PARENT, pstrNewName, keyDest) );
   DWORD dwRes = ::RegCopyTree(keySrc, NULL, keyDest);
   if( dwRes != ERROR_SUCCESS ) ::RegDeleteKey(keyParent, pstrNewName);
   else dwRes = ::RegDeleteTree(keyParent, m_pRegInfo->cName);
   if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
   return S_OK;
}

/**
 * Delete this item.
 */
HRESULT CRegItemKey::Delete()
{
   CRegKey key;
   HR( _OpenRegKey(REGACC_OPEN, DELETE|KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, REGPATH_PARENT, NULL, key) );
   DWORD dwRes = ::RegDeleteTree(key, m_pRegInfo->cName);
   return AtlHresultFromWin32(dwRes);
}

// Static members

/**
 * Serialize item from static data.
 * The Shell sometimes uses this method when it wants to create a brand
 * new folder.
 */
PCITEMID_CHILD CRegItemKey::GenerateITEMID(const WIN32_FIND_DATA& wfd)
{
   // Serialize data from a WIN32_FIND_DATA structure to a child PIDL.
   REGKEYPIDLINFO data = { 0 };
   data.magic = REG_MAGICID_KEY;
   wcscpy_s(data.cName, lengthof(data.cName), wfd.cFileName);
   data.ftLastWrite = wfd.ftLastWriteTime;
   return CNseBaseItem::GenerateITEMID(&data, sizeof(data));
}

/**
 * Serialize item from static data.
 */
PCITEMID_CHILD CRegItemKey::GenerateITEMID(const REGKEYPIDLINFO& src)
{
   // Serialize data from our REGKEYPIDLINFO structure to a child PIDL.
   REGKEYPIDLINFO data = src;
   data.magic = REG_MAGICID_KEY;
   return CNseBaseItem::GenerateITEMID(&data, sizeof(data));
}

// Implementation

/**
 * Is this the root level of the Registry branches?
 */
BOOL CRegItemKey::_IsTopLevel() const
{
   return ::ILIsEmpty(m_pidlFolder);
}

/**
 * Return the registry root and path.
 * This method returns the path for the key that can be used with the
 * RegOpenKey() API family.
 */
HRESULT CRegItemKey::_GetRegPathQuick(PCIDLIST_RELATIVE pidlPath, PCITEMID_CHILD pidlChild, HKEY& hKeyRoot, LPWSTR pszPath) const
{
   if( ::ILIsEmpty(pidlPath) ) {
      pidlPath = pidlChild;
      pidlChild = NULL;
   }
   if( !::ILIsEmpty(pidlPath) ) {
      CPidlMemPtr<REGKEYPIDLINFO> pInfo = pidlPath;
      if( wcscmp(pInfo->cName, L"HKEY_CLASSES_ROOT") == 0 ) hKeyRoot = HKEY_CLASSES_ROOT;
      else if( wcscmp(pInfo->cName, L"HKEY_DYN_DATA") == 0 ) hKeyRoot = HKEY_DYN_DATA;
      else if( wcscmp(pInfo->cName, L"HKEY_CURRENT_USER") == 0 ) hKeyRoot = HKEY_CURRENT_USER;
      else if( wcscmp(pInfo->cName, L"HKEY_LOCAL_MACHINE") == 0 ) hKeyRoot = HKEY_LOCAL_MACHINE;
      else if( wcscmp(pInfo->cName, L"HKEY_CURRENT_CONFIG") == 0 ) hKeyRoot = HKEY_CURRENT_CONFIG;
      else if( wcscmp(pInfo->cName, L"HKEY_PERFORMANCE_DATA") == 0 ) hKeyRoot = HKEY_PERFORMANCE_DATA;
      pidlPath = static_cast<PCIDLIST_RELATIVE>(::ILNext(pidlPath));
   }
   while( !::ILIsEmpty(pidlPath) ) {
      CPidlMemPtr<REGKEYPIDLINFO> pInfo = pidlPath;
      ::PathAppend(pszPath, pInfo->cName);
      pidlPath = static_cast<PCIDLIST_RELATIVE>(::ILNext(pidlPath));
   }
   if( !::ILIsEmpty(pidlChild) ) {
      CPidlMemPtr<REGKEYPIDLINFO> pInfo = pidlChild;
      ::PathAppend(pszPath, pInfo->cName);
   }
   return S_OK;
}

/**
 * Open or create the registry key behind this item.
 * Returns a CRegKey initialized with the HKEY of this folder item.
 */
HRESULT CRegItemKey::_OpenRegKey(REGOPEN_ACC Type, UINT uAccess, REGOPEN_PATH PathPart, LPCTSTR pszSubKey, CRegKey& key) const
{
   WCHAR wszPath[MAX_REGPATH] = { 0 };
   HKEY hKeyRoot = HKEY_CURRENT_USER;
   _GetRegPathQuick(m_pidlFolder, PathPart == REGPATH_FULLITEM ? m_pidlItem : NULL, hKeyRoot, wszPath);
   if( pszSubKey != NULL ) ::PathAppend(wszPath, pszSubKey);
   DWORD dwRes;
   if( Type == REGACC_CREATE ) dwRes = key.Create(hKeyRoot, wszPath, NULL, 0, uAccess);
   else dwRes = key.Open(hKeyRoot, wszPath, uAccess);
   return AtlHresultFromWin32(dwRes);
}

/**
 * Create a new key.
 * The method is called in response to the "New Folder" menu command.
 */
HRESULT CRegItemKey::_DoNewKey(const VFS_MENUCOMMAND& Cmd, UINT uLabelRes)
{
   CComBSTR bstrLabel;
   bstrLabel.LoadString(uLabelRes);
   CRegKey key;
   HR( _OpenRegKey(REGACC_CREATE, KEY_WRITE, REGPATH_FULLITEM, bstrLabel, key) );
   // Go into edit-mode for new item
   HR( _AddSelectEdit(Cmd, bstrLabel) );
   return S_OK;
}

/**
 * Create a new value.
 * The method is called in response to one of the "New -> Value (XXX)" range
 * of menu commands.
 */
HRESULT CRegItemKey::_DoNewValue(const VFS_MENUCOMMAND& Cmd, UINT uLabelRes, DWORD dwType)
{
   CComBSTR bstrLabel;
   bstrLabel.LoadString(uLabelRes);
   CRegKey key;
   HR( _OpenRegKey(REGACC_OPEN, KEY_WRITE, REGPATH_FULLITEM, NULL, key) );
   BYTE aData[10] = { 0 };
   DWORD cbData = 0;
   switch( dwType ) {
   case REG_DWORD:     cbData = sizeof(ULONG); break;
   case REG_QWORD:     cbData = sizeof(ULONGLONG); break;
   case REG_MULTI_SZ:  cbData = sizeof(WCHAR) * 2; break;
   case REG_BINARY:    cbData = 1; break;
   default:            cbData = sizeof(WCHAR); break;
   }
   ATLASSERT(cbData <= sizeof(aData));
   DWORD dwRes = ::RegSetKeyValue(key, NULL, bstrLabel, dwType, aData, cbData);
   if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
   // Add item to view and begin rename if it's not a (default) value
   DWORD dwFlags = SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_SELECT;
   if( uLabelRes != IDS_EMPTY ) dwFlags |= SVSI_EDIT;
   HR( _AddSelectEdit(Cmd, bstrLabel, dwFlags) );
   return S_OK;
}

