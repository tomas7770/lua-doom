#ifndef __D_LUA_MOBJ__
#define __D_LUA_MOBJ__

#include <lua.h>

#include "p_mobj.h"

mobj_t** NewMobj(lua_State* L, mobj_t* mobj);
void LoadMobjMetatable(lua_State* L);

#endif
