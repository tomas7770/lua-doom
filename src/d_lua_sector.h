#ifndef __D_LUA_SECTOR__
#define __D_LUA_SECTOR__

#include <lua.h>

#include "r_defs.h"

sector_t** NewSector(lua_State* L, sector_t* sector);
sector_t** CheckSectorInIndex(lua_State* L, int index);
void LoadSectorMetatable(lua_State* L);

#endif
