#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"
#include "d_lua_line.h"
#include "d_lua_sector.h"

#define LINE_META "LuaDoom.Line"

line_t** NewLine(lua_State* L, line_t* line) {
    return (line_t**) NewUserdata(L, (void*) line, sizeof(line_t*), LINE_META);
}

line_t** CheckLineInIndex(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, LINE_META);
    luaL_argcheck(L, ud != NULL, index, "'line' expected");
    return (line_t**) ud;
}

static line_t** CheckLine(lua_State* L) {
    return CheckLineInIndex(L, 1);
}

static int l_lineIndex(lua_State* L) {
    line_t** line_lua = CheckLine(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "special") == 0) {
        lua_pushinteger(L, (*line_lua)->special);
    }
    else if (strcmp(key, "tag") == 0) {
        lua_pushinteger(L, (*line_lua)->tag);
    }
    else if (strcmp(key, "flags") == 0) {
        lua_pushinteger(L, (*line_lua)->flags);
    }
    else if (strcmp(key, "frontsector") == 0) {
        NewSector(L, (*line_lua)->frontsector);
    }
    else if (strcmp(key, "backsector") == 0) {
        NewSector(L, (*line_lua)->backsector);
    }
    else {
        int v_type = luaL_getmetafield(L, 1, key);
        if (v_type == LUA_TNIL) {
            luaL_argerror(L, 2, "invalid line attribute");
        }
    }

    return 1;
}

static const struct luaL_Reg line_lib[] = {
    {"__index", l_lineIndex},
    {NULL, NULL}
};

void LoadLineMetatable(lua_State* L) {
    luaL_newmetatable(L, LINE_META);

    luaL_setfuncs(L, line_lib, 0);
}
