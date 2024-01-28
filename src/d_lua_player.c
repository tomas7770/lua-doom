#include <lauxlib.h>
#include <lualib.h>

#include "d_lua_player.h"
#include "d_lua_mobj.h"

#define PLAYER_META "LuaDoom.Player"

player_t** NewPlayer(lua_State* L, player_t* player) {
    player_t** player_lua = (player_t**) lua_newuserdata(L, sizeof(player_t*));
    luaL_getmetatable(L, PLAYER_META);
    lua_setmetatable(L, -2); // set player metatable
    *player_lua = player;
    return player_lua;
}

static player_t** CheckPlayerInIndex(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, PLAYER_META);
    luaL_argcheck(L, ud != NULL, index, "'player' expected");
    return (player_t**) ud;
}

static player_t** CheckPlayer(lua_State* L) {
    return CheckPlayerInIndex(L, 1);
}

static int l_playerIndex(lua_State* L) {
    player_t** player_lua = CheckPlayer(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "mo") == 0) {
        if ((*player_lua)->mo) {
            NewMobj(L, (*player_lua)->mo);
        }
        else {
            lua_pushnil(L);
        }
    }
    else {
        int v_type = luaL_getmetafield(L, 1, key);
        if (v_type == LUA_TNIL) {
            luaL_argerror(L, 2, "invalid player attribute");
        }
    }

    return 1;
}

static int l_playerEq(lua_State* L) {
    player_t** player_lua = CheckPlayer(L);
    player_t** player2_lua = CheckPlayerInIndex(L, 2);
    lua_pushboolean(L, *player_lua == *player2_lua);
    return 1;
}

static const struct luaL_Reg player_lib[] = {
    {"__index", l_playerIndex},
    {"__eq", l_playerEq},
    {NULL, NULL}
};

void LoadPlayerMetatable(lua_State* L) {
    luaL_newmetatable(L, PLAYER_META);

    luaL_setfuncs(L, player_lib, 0);
}
