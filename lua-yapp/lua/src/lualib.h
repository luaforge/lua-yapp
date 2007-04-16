/*
** $Id: lualib.h,v 1.1 2007-04-16 18:51:57 gsittig Exp $
** Lua standard libraries
** See Copyright Notice in lua.h
*/


#ifndef lualib_h
#define lualib_h

#include "lua.h"


/* Key to file-handle type */
#define LUA_FILEHANDLE		"FILE*"

LUALIB_API int (luaopen_base) (lua_State *L) CSEC_LBASELIB;
LUALIB_API int (luaopen_table) (lua_State *L) CSEC_LTABLIB;
LUALIB_API int (luaopen_io) (lua_State *L) CSEC_LIOLIB;
LUALIB_API int (luaopen_os) (lua_State *L) CSEC_LOSLIB;
LUALIB_API int (luaopen_string) (lua_State *L) CSEC_LSTRLIB;
LUALIB_API int (luaopen_math) (lua_State *L) CSEC_LMATHLIB;
LUALIB_API int (luaopen_debug) (lua_State *L) CSEC_LDBLIB;
LUALIB_API int (luaopen_package) (lua_State *L) CSEC_LOADLIB;
LUALIB_API void (luaL_openlibs) (lua_State *L) CSEC_LINIT; 

#define LUA_COLIBNAME	"coroutine"
LUALIB_API int (luaopen_base) (lua_State *L);

#define LUA_TABLIBNAME	"table"
LUALIB_API int (luaopen_table) (lua_State *L);

#define LUA_IOLIBNAME	"io"
LUALIB_API int (luaopen_io) (lua_State *L);

#define LUA_OSLIBNAME	"os"
LUALIB_API int (luaopen_os) (lua_State *L);

#define LUA_STRLIBNAME	"string"
LUALIB_API int (luaopen_string) (lua_State *L);

#define LUA_MATHLIBNAME	"math"
LUALIB_API int (luaopen_math) (lua_State *L);

#define LUA_DBLIBNAME	"debug"
LUALIB_API int (luaopen_debug) (lua_State *L);

#define LUA_LOADLIBNAME	"package"
LUALIB_API int (luaopen_package) (lua_State *L);


/* open all previous libraries */
LUALIB_API void (luaL_openlibs) (lua_State *L); 



#ifndef lua_assert
#define lua_assert(x)	((void)0)
#endif


#endif
