#ifndef __D_LUA_LINE__
#define __D_LUA_LINE__

#include <lua.h>

#include "r_defs.h"

line_t** NewLine(lua_State* L, line_t* line);
line_t** CheckLineInIndex(lua_State* L, int index);
void LoadLineMetatable(lua_State* L);

#endif
