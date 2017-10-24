
#include "stdafx.h"

#include "RegFileSystem.h"
#include "ShellFolder.h"


///////////////////////////////////////////////////////////////////////////////
// CRegItemValue

CRegItemValue::CRegItemValue(CShellFolder* pFolder, PCIDLIST_RELATIVE pidlFolder, PCITEMID_CHILD pidlItem, BOOL bReleaseItem) :
   CNseBaseItem(pFolder, pidlFolder, pidlItem, bReleaseItem)
{
   // Extract item data
   m_pRegInfo = reinterpret_cast<const REGKEYPIDLINFO*>(pidlItem);
}

/**
 * Get the item type.
 */
BYTE CRegItemValue::GetType()
{
   return REG_MAGICID_VALUE;   
}

/**
 * Create Shell object for extracting icon and image.
 * We should create the Shell object asked for by the Shell to generate
 * the display icon or thumbnail image.
 */
HRESULT CRegItemValue::GetExtractIcon(REFIID riid, LPVOID* ppRetVal)
{
   // Use our SHCreateModuleExtractIcon() method to create a default object
   // for the icon. We won't support IID_IExtractImage here.
   int iIconIndex = IDI_VALUE_BIN;
   switch( m_pRegInfo->dwValueType ) {
   case REG_SZ:
   case REG_MULTI_SZ:
   case REG_EXPAND_SZ:
      iIconIndex = IDI_VALUE_TEXT;
      break;
   }
   return ::SHCreateModuleExtractIcon(iIconIndex, riid, ppRetVal);
}

/**
 * Return item information.
 * We support the properties for the columns as well as a number of
 * administrative information bits (such as what properties to display
 * in the Details panel).
 */
HRESULT CRegItemValue::GetProperty(REFPROPERTYKEY pkey, CComPropVariant& v)
{
   if( pkey == PKEY_ItemName ) {
      return ::InitPropVariantFromString(m_pRegInfo->cName, &v);
   }
   if( pkey == PKEY_ParsingName ) {
      if( m_pRegInfo->cName[0] == '\0' ) {
         CComBSTR bstr; 
         bstr.LoadString(IDS_REGVAL_DEFAULT);
         return ::InitPropVariantFromString(bstr, &v);
      }
      return ::InitPropVariantFromString(m_pRegInfo->cName, &v);
   }
   if( pkey == PKEY_ItemNameDisplay ) {
      if( m_pRegInfo->cName[0] == '\0' ) {
         CComBSTR bstr; 
         bstr.LoadString(IDS_REGVAL_DEFAULT);
         return ::InitPropVariantFromString(bstr, &v);
      }
      return ::InitPropVariantFromString(m_pRegInfo->cName, &v);
   }
   // Return values for our custom Registry properties. To expose these we
   // added the VFS_INSTALL_PROPERTIES flag and a Properties.xslt script that
   // defines the properties. We also reference them in the PropList items
   // below to make sure they appear in the Detail panel.
   if( pkey == PKEY_RegistryType ) {
      return ::InitPropVariantFromUInt32(1UL, &v);
   }
   if( pkey == PKEY_RegistryValueType ) {
      return ::InitPropVariantFromUInt32(m_pRegInfo->dwValueType, &v);
   }
   if( pkey == PKEY_RegistryValue ) {
      return _GetRegValue(v);
   }
   // Return properties for display details. These define what properties to
   // display in various places in the Shell, such as the hover-tip and Details panel.
   if( pkey == PKEY_PropList_TileInfo ) {
      return ::InitPropVariantFromString(L"prop:Windows.Registry.Type;", &v);
   }
   if( pkey == PKEY_PropList_ExtendedTileInfo ) {
      return ::InitPropVariantFromString(L"prop:Windows.Registry.Type;Windows.Registry.ValueType;", &v);
   }
   if( pkey == PKEY_PropList_PreviewTitle ) {
      return ::InitPropVariantFromString(L"prop:Windows.Registry.Type;Windows.Registry.ValueType;", &v);
   }
   if( pkey == PKEY_PropList_PreviewDetails ) {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;Windows.Registry.ValueType;Windows.Registry.Value;", &v);
   }
   if( pkey == PKEY_PropList_InfoTip ) {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;Windows.Registry.Type;Windows.Registry.ValueType;Windows.Registry.Value;", &v);
   }
   if( pkey == PKEY_PropList_FullDetails ) {
      return ::InitPropVariantFromString(L"prop:System.ItemNameDisplay;Windows.Registry.Type;Windows.Registry.ValueType;Windows.Registry.Value;", &v);
   }      
   return CNseBaseItem::GetProperty(pkey, v);
}

/**
 * Set a property on the item.
 */
HRESULT CRegItemValue::SetProperty(REFPROPERTYKEY pkey, const CComPropVariant& v)
{
   if( pkey == PKEY_RegistryValue ) {
      return _SetRegValue(v);
   }
   return CNseBaseItem::SetProperty(pkey, v);
}

/**
 * Return SFGAOF flags for this item.
 * These flags tells the Shell about the capabilities of this item. Here we
 * can toggle the ability to copy/delete/rename an item.
 */
SFGAOF CRegItemValue::GetSFGAOF(SFGAOF dwMask)
{
   // We support DELETE and RENAME of values, and not really a Property Sheet.
   // Unfortunately the Shell won't display writeable properties without the
   // sheet flag!?
   return SFGAO_CANDELETE 
          | SFGAO_CANRENAME
          | SFGAO_HASPROPSHEET;
}

/**
 * Return file information.
 * We use this to return a simple structure with key information about
 * our item.
 */
VFS_FIND_DATA CRegItemValue::GetFindData()
{
   VFS_FIND_DATA wfd = { 0 };
   wcscpy_s(wfd.cFileName, lengthof(wfd.cFileName), m_pRegInfo->cName);
   wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_VIRTUAL;
   return wfd;
}

/**
 * Rename this item.
 */
HRESULT CRegItemValue::Rename(LPCWSTR pstrNewName, LPWSTR pstrOutputName)
{
   CRegKey key;
   HR( _OpenRegKey(KEY_ALL_ACCESS, key) );
   // Read existing value, write a new value and delete the old...
   DWORD dwType = 0, cbData = 0;
   DWORD dwRes = ::RegQueryValueEx(key, m_pRegInfo->cName, NULL, &dwType, NULL, &cbData);
   if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
   CAutoVectorPtr<BYTE> buffer( new BYTE[cbData] );
   dwRes = ::RegQueryValueEx(key, m_pRegInfo->cName, NULL, &dwType, buffer, &cbData);
   if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
   dwRes = ::RegSetValueEx(key, pstrNewName, 0, dwType, buffer, cbData);
   if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
   dwRes = ::RegDeleteValue(key, m_pRegInfo->cName);
   if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
   return S_OK;
}

/**
 * Delete this item.
 */
HRESULT CRegItemValue::Delete()
{
   CRegKey key;
   HR( _OpenRegKey(KEY_WRITE|DELETE, key) );
   DWORD dwRes = ::RegDeleteValue(key, m_pRegInfo->cName);
   return AtlHresultFromWin32(dwRes);
}

// Static members

/**
 * Serialize item from static data.
 */
PCITEMID_CHILD CRegItemValue::GenerateITEMID(const WIN32_FIND_DATA& wfd)
{
   // Serialize data from a WIN23_FIND_DATA structure to a child PIDL.
   REGKEYPIDLINFO data = { 0 };
   data.magic = REG_MAGICID_VALUE;
   wcscpy_s(data.cName, lengthof(data.cName), wfd.cFileName);
   // Hotfix of (default) item; which is internally known as '\0'
   CComBSTR bstrDefault;
   bstrDefault.LoadString(IDS_REGVAL_DEFAULT);
   if( bstrDefault == data.cName ) data.cName[0] = '\0';
   // Return serialized data
   return CNseBaseItem::GenerateITEMID(&data, sizeof(data));
}

/**
 * Serialize item from static data.
 */
PCITEMID_CHILD CRegItemValue::GenerateITEMID(const REGKEYPIDLINFO& src)
{
   // Serialize data from our REGKEYPIDLINFO structure to a child PIDL.
   REGKEYPIDLINFO data = src;
   data.magic = REG_MAGICID_VALUE;
   return CNseBaseItem::GenerateITEMID(&data, sizeof(data));
}

// Implementation

HRESULT CRegItemValue::_GetPathnameQuick(PCIDLIST_RELATIVE pidlPath, PCITEMID_CHILD pidlChild, BOOL bParsingPath, HKEY& hKeyRoot, LPWSTR pszPath) const
{
   if( !::ILIsEmpty(pidlPath) ) {
      CPidlMemPtr<REGKEYPIDLINFO> pInfo = pidlPath;
      if( bParsingPath ) ::PathAppend(pszPath, pInfo->cName);
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
 * Get the HKEY for accessing the item's value.
 * Retrieves the registry key (HKEY) on which the value is attached.
 */
HRESULT CRegItemValue::_OpenRegKey(UINT uAccess, CRegKey& key) const
{
   HKEY hKeyRoot = HKEY_CURRENT_USER;
   WCHAR wszPath[MAX_REGPATH] = { 0 };
   _GetPathnameQuick(m_pidlFolder, NULL, FALSE, hKeyRoot, wszPath);
   DWORD dwRes = key.Open(hKeyRoot, wszPath, uAccess);
   return AtlHresultFromWin32(dwRes);
}

/**
 * Get the value of the registry item.
 */
HRESULT CRegItemValue::_GetRegValue(CComPropVariant& v) const
{
   CRegKey key;
   HR( _OpenRegKey(KEY_QUERY_VALUE, key) );
   switch( m_pRegInfo->dwValueType ) {
   case REG_SZ:
      {
         DWORD cbData = 0;
         DWORD dwRes = ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_SZ, NULL, &cbData, &cbData);
         if( dwRes != ERROR_MORE_DATA ) return AtlHresultFromWin32(dwRes);
         CAutoVectorPtr<BYTE> buffer( new BYTE[cbData] );
         ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, NULL, buffer, &cbData);
         return ::InitPropVariantFromString((LPCWSTR)(buffer.m_p), &v);
      }
   case REG_EXPAND_SZ:
      {
         DWORD cbData = 0;
         DWORD dwRes = ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, NULL, &cbData, &cbData);
         if( dwRes != ERROR_MORE_DATA ) return AtlHresultFromWin32(dwRes);
         CAutoVectorPtr<BYTE> buffer( new BYTE[cbData] );
         ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND | RRF_ZEROONFAILURE, NULL, buffer, &cbData);
         return ::InitPropVariantFromString((LPCWSTR)(buffer.m_p), &v);
      }
   case REG_DWORD:
   case REG_DWORD_BIG_ENDIAN:
      {
         DWORD dwValue = 0;
         DWORD cbData = sizeof(dwValue);
         DWORD dwRes = ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_DWORD, NULL, &dwValue, &cbData);
         if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
         return ::InitPropVariantFromUInt32(dwValue, &v);
      }
   case REG_QWORD:
      {
         ULONGLONG ullValue = 0;
         DWORD cbData = sizeof(ullValue);
         DWORD dwRes = ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_QWORD, NULL, &ullValue, &cbData);
         if( dwRes != ERROR_SUCCESS ) return AtlHresultFromWin32(dwRes);
         return ::InitPropVariantFromUInt64(ullValue, &v);
      }
   case REG_BINARY:
      {
         DWORD cbData = 0;
         DWORD dwRes = ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_BINARY, NULL, &cbData, &cbData);
         if( dwRes != ERROR_MORE_DATA ) return AtlHresultFromWin32(dwRes);
         CAutoVectorPtr<BYTE> buffer( new BYTE[cbData] );
         ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_BINARY | RRF_ZEROONFAILURE, NULL, buffer, &cbData);
         return ::InitPropVariantFromBuffer(buffer, cbData, &v);
      }
   case REG_MULTI_SZ:
      {
         DWORD cbData = 0;
         DWORD dwRes = ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_MULTI_SZ, NULL, &cbData, &cbData);
         if( dwRes != ERROR_MORE_DATA ) return AtlHresultFromWin32(dwRes);
         CAutoVectorPtr<BYTE> buffer( new BYTE[cbData] );
         ::RegGetValue(key, NULL, m_pRegInfo->cName, RRF_RT_REG_MULTI_SZ | RRF_ZEROONFAILURE, NULL, buffer, &cbData);
         CSimpleValArray<LPCWSTR> aList;
         for( LPCWSTR p = (LPWSTR) buffer.m_p; *p != '\0'; p += wcslen(p) + 1 ) aList.Add(p);
         return ::InitPropVariantFromStringVector(aList.GetData(), aList.GetSize(), &v);
      }
   }
   return E_FAIL;
}

/**
 * Set the value of the registry item.
 */
HRESULT CRegItemValue::_SetRegValue(const CComPropVariant& vIn) const
{
   CComPropVariant v = vIn;
   CRegKey key;
   HR( _OpenRegKey(KEY_WRITE, key) );
   switch( m_pRegInfo->dwValueType ) {
   case REG_SZ:
   case REG_EXPAND_SZ:
      {
         HR( v.ChangeType(VT_LPWSTR) );
         DWORD dwRes = ::RegSetValueEx(key, m_pRegInfo->cName, NULL, m_pRegInfo->dwValueType, (LPBYTE)(v.pwszVal), (DWORD)(wcslen(v.pwszVal) + 1) * sizeof(WCHAR));
         return AtlHresultFromWin32(dwRes);
      }
   case REG_DWORD:
   case REG_DWORD_BIG_ENDIAN:
      {
         HR( v.ChangeType(VT_UI4) );
         DWORD dwRes = ::RegSetValueEx(key, m_pRegInfo->cName, NULL, m_pRegInfo->dwValueType, (LPBYTE)(&v.ulVal), sizeof(DWORD));
         return AtlHresultFromWin32(dwRes);
      }
   case REG_QWORD:
      {
         HR( v.ChangeType(VT_UI8) );
         DWORD dwRes = ::RegSetValueEx(key, m_pRegInfo->cName, NULL, m_pRegInfo->dwValueType, (LPBYTE)(&v.uhVal.QuadPart), sizeof(ULONGLONG));
         return AtlHresultFromWin32(dwRes);
      }
   case REG_MULTI_SZ:
      {
         HR( v.ChangeType(VT_LPWSTR|VT_VECTOR) );
         LPWSTR* ppStrings = NULL;
         ULONG nStrings = 0;
         HR( ::PropVariantToStringVectorAlloc(v, &ppStrings, &nStrings) );
         ULONG cbData = sizeof(WCHAR);
         for( ULONG i = 0; i < nStrings; i++ ) {
            DWORD cbString = (DWORD)(wcslen(ppStrings[i]) + 1) * sizeof(WCHAR);
            cbData += cbString;
         }
         CAutoVectorPtr<BYTE> buffer( new BYTE[cbData] );
         LPWSTR p = reinterpret_cast<LPWSTR>(buffer.m_p);
         for( ULONG i = 0; i < nStrings; i++ ) {
            SIZE_T cchString = wcslen(ppStrings[i]) + 1;
            ::CopyMemory(p, ppStrings[i], cchString * sizeof(WCHAR));
            ::CoTaskMemFree(ppStrings[i]);
            p += cchString;
         }
         ::CoTaskMemFree(ppStrings);
         *p = '\0';
         DWORD dwRes = ::RegSetValueEx(key, m_pRegInfo->cName, NULL, m_pRegInfo->dwValueType, buffer, cbData);
         return AtlHresultFromWin32(dwRes);
      }
   }
   return E_FAIL;
}

