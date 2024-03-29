#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"
#include "d_lua_sector.h"

#define SECTOR_META "LuaDoom.Sector"

sector_t** NewSector(lua_State* L, sector_t* sector) {
    return (sector_t**) NewUserdata(L, (void*) sector, sizeof(sector_t*), SECTOR_META);
}

sector_t** CheckSectorInIndex(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, SECTOR_META);
    luaL_argcheck(L, ud != NULL, index, "'sector' expected");
    return (sector_t**) ud;
}

static sector_t** CheckSector(lua_State* L) {
    return CheckSectorInIndex(L, 1);
}

static int l_sectorIndex(lua_State* L) {
    sector_t** sector_lua = CheckSector(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "floorheight") == 0) {
        lua_pushnumber(L, FromFixed((*sector_lua)->floorheight));
    }
    else if (strcmp(key, "ceilingheight") == 0) {
        lua_pushnumber(L, FromFixed((*sector_lua)->ceilingheight));
    }
    else if (strcmp(key, "floorpic") == 0) {
        lua_pushinteger(L, (*sector_lua)->floorpic);
    }
    else if (strcmp(key, "ceilingpic") == 0) {
        lua_pushinteger(L, (*sector_lua)->ceilingpic);
    }
    else {
        int v_type = luaL_getmetafield(L, 1, key);
        if (v_type == LUA_TNIL) {
            luaL_argerror(L, 2, "invalid sector attribute");
        }
    }

    return 1;
}

static const struct luaL_Reg sector_lib[] = {
    {"__index", l_sectorIndex},
    {NULL, NULL}
};

void LoadSectorMetatable(lua_State* L) {
    luaL_newmetatable(L, SECTOR_META);

    luaL_setfuncs(L, sector_lib, 0);
}
