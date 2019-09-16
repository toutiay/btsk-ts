#ifndef _LUAWRAPPER_H_
#define _LUAWRAPPER_H_

extern "C" {
	#include "..\include\lua.h"
	#include "..\include\lualib.h"
	#include "..\include\lauxlib.h"
}

#include <string>
#include <iostream>

class LuaWrapper {
public:
  static LuaWrapper* instance() { if (!m_wrapper) { m_wrapper = new LuaWrapper(); } return m_wrapper; }

  void load_all_scripts();

  void call_function (const char * table, const char* name);
  bool call_function_with_bool_return (const char* table, const char* name);


private:
  LuaWrapper() { L = lua_open(); luaL_openlibs(L); } 
  ~LuaWrapper() { lua_close(L); delete m_wrapper; }
private:
  static LuaWrapper* m_wrapper;

  lua_State *L;
};

#endif //_LUAWRAPPER_H_