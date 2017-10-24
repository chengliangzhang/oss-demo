// SOWFolder.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"


CAtlDllModule _AtlModule;
CSOWShellModule _ShellModule;


// DEBUGGING NOTES:
//  Remember to register the DLL using the regsvr32 utility or similar.
//  To debug in MS Visual Studio, kill all running Explorer.exe processes and set the
//  following debug settings for the project:
//   Command:                    C:\windows\explorer.exe


// These are the GUIDs that identify the TarFolder shell junction and friends.
// Make sure to choose unique GUIDs for every project.
const CLSID CLSID_ShellFolder     =     {0XC3D7BC7E, 0X7DDC, 0X436C, { 0XB8, 0X7C, 0XE0, 0XB2, 0XD3, 0XE3, 0XFE, 0XBE}};
const CLSID CLSID_SendTo          =     {0XBFC68A32, 0XC1A3, 0X4DCE, { 0X85, 0XEB, 0X8E, 0X9E, 0X0F, 0XE3, 0X9A, 0X09}};
const CLSID CLSID_Preview         =     {0X85127494, 0X326F, 0X425B, { 0XBF, 0X2F, 0X0E, 0X6A, 0X48, 0X30, 0X3F, 0XC6}};
const CLSID CLSID_DropTarget      =     {0X66DB038C, 0X6BA1, 0X4145, { 0XA8, 0X0A, 0X5B, 0XB4, 0X01, 0X87, 0XA8, 0X61}};
const CLSID CLSID_ContextMenu     =     {0X1691ECD8, 0X267E, 0X4871, { 0XBB, 0XE1, 0X3C, 0X96, 0X2E, 0X6D, 0X2A, 0XAF}};
const CLSID CLSID_PropertySheet   =     {0X5214197E, 0X4F51, 0X496E, { 0X9B, 0X5E, 0XE5, 0X5B, 0XFB, 0X1A, 0X4A, 0XEC}};
