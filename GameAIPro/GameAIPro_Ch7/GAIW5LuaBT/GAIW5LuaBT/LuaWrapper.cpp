#include "LuaWrapper.h"

#include <Windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>

#include <assert.h>

#include "Behaviors\BehaviorMgr.h"

LuaWrapper* LuaWrapper::m_wrapper = NULL;

void DisplayErrorBox(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and clean up

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

//This function copies heavily from http://msdn.microsoft.com/en-us/library/aa365200(VS.85).aspx
// The basic gist is using Windows APIs to find every "*.lua" script in the scripts directory.
// The end user can replace this for different OS's if necessary, or use a different method to 
// load the scripts entirely.
void LuaWrapper::load_all_scripts()
{
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;
   
  // Prepare string for use with FindFile functions.  First, copy the
  // string to a buffer, then append '\*' to the directory name.

  StringCchCopy(szDir, MAX_PATH, "Scripts");
  StringCchCat(szDir, MAX_PATH, TEXT("\\*.lua"));

  // Find the first file in the directory.
  hFind = FindFirstFile(szDir, &ffd);

  if (INVALID_HANDLE_VALUE == hFind)
  {
     DisplayErrorBox(TEXT("FindFirstFile"));
     return;
  } 
   
  do
  {
    if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
	    std::string full_path = "Scripts\\";
	    full_path.append(ffd.cFileName);
		  if (luaL_dofile(L, full_path.c_str())) {
			  std::cerr << "Error loading lua file: " << full_path.c_str() << std::endl;
		  } else {
        BehaviorMgr::instance()->add_behavior(ffd.cFileName);
      }
    }
  }
  while (FindNextFile(hFind, &ffd) != 0);
 
  dwError = GetLastError();
  if (dwError != ERROR_NO_MORE_FILES) 
  {
     DisplayErrorBox(TEXT("FindFirstFile"));
  }

  FindClose(hFind);
}

void LuaWrapper::call_function (const char* table, const char* name)
{
  lua_getglobal(L, table);
  lua_getfield(L, -1, name);
  lua_call(L, 0, 0);
  lua_pop(L, 1);
  assert(0 == lua_gettop(L));
}

bool LuaWrapper::call_function_with_bool_return (const char* table, const char* name)
{
  lua_getglobal(L, table);
  lua_getfield(L, -1, name);
  lua_call(L, 0, 1);

  bool res = true;
  if (lua_isboolean(L, -1)) {
    res = (lua_toboolean(L, -1) == 1 ? true : false);
  }
  lua_pop(L, 2);
  assert(0 == lua_gettop(L));

  return res;
}