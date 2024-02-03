#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"
#include "d_lua_player.h"
#include "d_lua_mobj.h"
#include "d_lua_pspr.h"

extern actionf_t FindDehCodepointer(const char* key);
extern char *ptr_lstrip(char *p);

extern lua_cptr lua_cptrs[];
extern size_t lua_cptrs_count;

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

static int l_player_call(lua_State* L) {
    char key[LUA_CPTR_NAME_SIZE];
    actionf_t c_cptr;
    player_t** player_lua = CheckPlayer(L);
    pspdef_t** psp_lua = CheckPsprInIndex(L, 2);
    const char* cptr_name = luaL_checkstring(L, 3);
    boolean found = false;
    int i = -1; // incremented to start at zero at the top of the loop
    
    strcpy(key,"A_");  // reusing the key area to prefix the mnemonic
    strcat(key, ptr_lstrip((char*) cptr_name));
    
    c_cptr = FindDehCodepointer(key);
    if (c_cptr.p2) {
        // Count args and apply them
        int j;
        long orig_args[MAXSTATEARGS];
        int extra_args = lua_gettop(L)-3;
        memcpy(orig_args, (*psp_lua)->state->args, sizeof(orig_args));

        for (j = 0; j < extra_args && j < MAXSTATEARGS; j++) {
            int value;
            if (lua_isnil(L, j+1+3)) {
                // Skip arg
                continue;
            }
            value = luaL_checkinteger(L, j+1+3);
            // Changes the state globally for all psprs of this type, quite a hack...
            // But atleast it's restored later
            (*psp_lua)->state->args[j] = value;
        }

        c_cptr.p2(*player_lua, *psp_lua);
        // Restore state args
        memcpy((*psp_lua)->state->args, orig_args, sizeof(orig_args));
        found = true;
    }
    else {
        // Look for Lua codepointer
        do
        {
            ++i;
            if (!strcasecmp(key, lua_cptrs[i].lookup))
            {
                CallLuaCptrP2(lua_cptrs[i].cptr, *player_lua, *psp_lua);
                found = true;
            }
        } while (!found && i < lua_cptrs_count);
    }

    if (!found) {
        luaL_argerror(L, 2, "codepointer not found");
    }

    return 0;
}

static int l_player_getCheat(lua_State* L) {
    player_t** player_lua = CheckPlayer(L);
    int cheat = luaL_checkinteger(L, 2);
    if (cheat < 0 || cheat > CF_NOTARGET) {
        luaL_argerror(L, 2, "invalid cheat");
    }
    lua_pushboolean(L, ((*player_lua)->cheats & cheat) ? true : false);
    return 1;
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
    {"call", l_player_call},
    {"getCheat", l_player_getCheat},
    {NULL, NULL}
};

void LoadPlayerMetatable(lua_State* L) {
    luaL_newmetatable(L, PLAYER_META);

    luaL_setfuncs(L, player_lib, 0);
}
