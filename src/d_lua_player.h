#ifndef __D_LUA_PLAYER__
#define __D_LUA_PLAYER__

#include <lua.h>

#include "d_player.h"

#define PLAYER_META "LuaDoom.Player"

player_t** NewPlayer(lua_State* L, player_t* player);
void LoadPlayerMetatable(lua_State* L);

#endif
