#include <lauxlib.h>
#include <lualib.h>

#include "d_lua_pspr.h"

#define PSPR_META "LuaDoom.Pspr"

pspdef_t** NewPspr(lua_State* L, pspdef_t* psp) {
    pspdef_t** psp_lua = (pspdef_t**) lua_newuserdata(L, sizeof(pspdef_t*));
    luaL_getmetatable(L, PSPR_META);
    lua_setmetatable(L, -2); // set pspr metatable
    *psp_lua = psp;
    return psp_lua;
}

pspdef_t** CheckPsprInIndex(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, PSPR_META);
    luaL_argcheck(L, ud != NULL, index, "'pspr' expected");
    return (pspdef_t**) ud;
}

static pspdef_t** CheckPspr(lua_State* L) {
    return CheckPsprInIndex(L, 1);
}

static int l_psprIndex(lua_State* L) {
    pspdef_t** psp_lua = CheckPspr(L);

    const char* key = luaL_checkstring(L, 2);
    {
        int v_type = luaL_getmetafield(L, 1, key);
        if (v_type == LUA_TNIL) {
            luaL_argerror(L, 2, "invalid pspr attribute");
        }
    }

    return 1;
}

static int l_psprEq(lua_State* L) {
    pspdef_t** psp_lua = CheckPspr(L);
    pspdef_t** psp2_lua = CheckPsprInIndex(L, 2);
    lua_pushboolean(L, *psp_lua == *psp2_lua);
    return 1;
}

static const struct luaL_Reg pspr_lib[] = {
    {"__index", l_psprIndex},
    {"__eq", l_psprEq},
    {NULL, NULL}
};

void LoadPsprMetatable(lua_State* L) {
    luaL_newmetatable(L, PSPR_META);

    luaL_setfuncs(L, pspr_lib, 0);
}
