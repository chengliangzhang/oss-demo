
#include "stdafx.h"

#include "SOWFileSystem.h"
#include "ShellUpdater.h"
#include "Utils.h"

CMsg g_Msg;
CShellUpdater g_Updater;
///////////////////////////////////////////////////////////////////////////////
// CSOWShellModule

/**
 * Return capabilities of this Shell Extension.
 * The GetConfigXXX methods are called by the NSE to query what functionality our
 * file-system supports.
 */
BOOL CSOWShellModule::GetConfigBool(VFS_CONFIG Item)
{
   switch( Item ) 
   {
   //case VFS_INSTALL_SENDTO:
   //case VFS_INSTALL_PREVIEW:
   //case VFS_INSTALL_PROPSHEET:
   //case VFS_INSTALL_DROPTARGET:
   //case VFS_INSTALL_CONTEXTMENU:
   //case VFS_INSTALL_CUSTOMSCRIPT:
   //case VFS_INSTALL_STARTMENU_LINK:
   //   return TRUE;

   case VFS_CAN_SLOW_COPY:
   //case VFS_CAN_SLOW_ENUM:
   case VFS_CAN_PROGRESSUI:
   case VFS_CAN_ATTACHMENTSERVICES:
      return TRUE;

   case VFS_HAVE_UNIQUE_NAMES:
   case VFS_HAVE_VIRTUAL_FILES:
   case VFS_HAVE_QUICKPARSENAME:
      return TRUE;

   case VFS_HAVE_ICONOVERLAYS:
	  return TRUE;
   }
   return FALSE;
}

/**
 * Return capabilities of this Shell Extension.
 * The GetConfigXXX methods are called by the NSE to query what functionality our
 * file-system supports.
 */
LONG CSOWShellModule::GetConfigInt(VFS_CONFIG Item)
{
   switch( Item ) 
   {
   case VFS_INT_LOCATION:
      return VFS_LOCATION_MYCOMPUTER;

   case VFS_INT_MAX_FILENAME_LENGTH:
      return SOW_MAXNAMELEN;

   case VFS_INT_MAX_PATHNAME_LENGTH:
      return SOW_MAXPATHLEN;

   case VFS_INT_SHELLROOT_SFGAO:
       return SFGAO_CANCOPY 
              | SFGAO_CANMOVE 
              | SFGAO_CANRENAME 
              //| SFGAO_DROPTARGET
              //| SFGAO_STREAM
              | SFGAO_FOLDER 
              | SFGAO_BROWSABLE 
              | SFGAO_HASSUBFOLDER 
              //| SFGAO_HASPROPSHEET 
              | SFGAO_FILESYSANCESTOR
              //| SFGAO_STORAGEANCESTOR
				;
   }
   return 0;
}

/**
 * Return capabilities of this Shell Extension.
 * The GetConfigXXX methods are called by the NSE to query what functionality our
 * file-system supports.
 */
LPCWSTR CSOWShellModule::GetConfigStr(VFS_CONFIG Item)
{
   switch( Item ) 
   {
   case VFS_STR_FILENAME_CHARS_NOTALLOWED:
      return L":<>\\/|\"'*?[]";

   }
   return NULL;
}

/**
 * Called during installation (dll registration)
 * Use this to do extra work during the module registration. Remember that this
 * method is run as Admin and only called during installation.
 */
HRESULT CSOWShellModule::DllInstall()
{
   return S_OK;
}

/**
 * Called during uninstall (dll de-registration)
 */
HRESULT CSOWShellModule::DllUninstall()
{
   return S_OK;
}

/**
 * Called on ShellNew integration.
 * Invoked by user through Desktop ContextMenu -> New -> Tar Folder.
 */
HRESULT CSOWShellModule::ShellAction(LPCWSTR pstrType, LPCWSTR pstrCmdLine)
{
   return S_OK;
}

/**
 * Called at process/thread startup/shutdown.
 */
BOOL CSOWShellModule::DllMain(DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

HRESULT CSOWShellModule::CreateFileSystem(PCIDLIST_ABSOLUTE pidlRoot, CNseFileSystem** ppFS)
{
   CSOWFileSystem* pFS = new CSOWFileSystem();
   HRESULT Hr = pFS->Init(pidlRoot);
   if( SUCCEEDED(Hr) ) *ppFS = pFS;
   else delete pFS; 
   return Hr;
}


///////////////////////////////////////////////////////////////////////////////
// CSOWFileSystem

CSOWFileSystem::CSOWFileSystem() : m_cRef(1)
{
}

CSOWFileSystem::~CSOWFileSystem()
{
	//if (g_pMsg)
	//{
	//	delete g_pMsg;
	//	g_pMsg = NULL;
	//}
}

HRESULT CSOWFileSystem::Init(PCIDLIST_ABSOLUTE pidlRoot)
{
	g_Msg.Connect();
	g_Updater.Initialise();
	return S_OK;
}

VOID CSOWFileSystem::AddRef()
{
   ::InterlockedIncrement(&m_cRef);
}

VOID CSOWFileSystem::Release()
{
   if( ::InterlockedDecrement(&m_cRef) == 0 ) delete this;
}

/**
 * Create the root NSE Item instance.
 */
CNseItem* CSOWFileSystem::GenerateRoot(CShellFolder* pFolder)
{
   return new CSOWFileItem(pFolder, NULL, NULL, TRUE);
}

