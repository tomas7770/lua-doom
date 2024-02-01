#ifndef __D_LUA_PSPR__
#define __D_LUA_PSPR__

#include <lua.h>

#include "p_pspr.h"

pspdef_t** NewPspr(lua_State* L, pspdef_t* psp);
void LoadPsprMetatable(lua_State* L);

#endif
