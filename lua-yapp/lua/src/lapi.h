/*
** $Id: lapi.h,v 1.1 2007-04-16 18:51:57 gsittig Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef lapi_h
#define lapi_h


#include "lobject.h"


LUAI_FUNC void luaA_pushobject (lua_State *L, const TValue *o) CSEC_LAPI;

LUAI_FUNC void luaA_pushobject (lua_State *L, const TValue *o);

#endif
