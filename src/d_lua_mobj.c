#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"
#include "d_lua_mobj.h"

#include "p_inter.h"
#include "p_tick.h"
#include "p_map.h"
#include "p_maputl.h"

extern actionf_t FindDehCodepointer(const char* key);
extern char *ptr_lstrip(char *p);

extern lua_cptr lua_cptrs[];
extern size_t lua_cptrs_count;

#define MOBJ_META "LuaDoom.Mobj"

mobj_t** NewMobj(lua_State* L, mobj_t* mobj) {
    mobj_t** mobj_lua = (mobj_t**) lua_newuserdata(L, sizeof(mobj_t*));
    luaL_getmetatable(L, MOBJ_META);
    lua_setmetatable(L, -2); // set mobj metatable
    *mobj_lua = mobj;
    return mobj_lua;
}

static mobj_t** CheckMobjInIndex(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, MOBJ_META);
    luaL_argcheck(L, ud != NULL, index, "'mobj' expected");
    return (mobj_t**) ud;
}

static mobj_t** CheckMobj(lua_State* L) {
    return CheckMobjInIndex(L, 1);
}

static int l_mobj_callGeneric(lua_State* L, boolean is_misc) {
    // Similar to the codepointer lookup code from d_deh.c
    char key[LUA_CPTR_NAME_SIZE];
    actionf_t c_cptr;
    mobj_t** mobj_lua = CheckMobj(L);
    const char* cptr_name = luaL_checkstring(L, 2);
    boolean found = false;
    int i = -1; // incremented to start at zero at the top of the loop
    
    strcpy(key,"A_");  // reusing the key area to prefix the mnemonic
    strcat(key, ptr_lstrip((char*) cptr_name));
    
    c_cptr = FindDehCodepointer(key);
    if (c_cptr.p1) {
        // Count args and apply them
        int j;
        long orig_args[MAXSTATEARGS];
        long orig_misc1, orig_misc2;
        int extra_args = lua_gettop(L)-2;
        state_t* orig_state = (*mobj_lua)->state;

        if (is_misc) {
            orig_misc1 = orig_state->misc1;
            orig_misc2 = orig_state->misc2;
        }
        else {
            memcpy(orig_args, orig_state->args, sizeof(orig_args));
        }

        for (j = 0; j < extra_args && j < (is_misc ? 2 : MAXSTATEARGS); j++) {
            int value;
            if (lua_isnil(L, j+1+2)) {
                // Skip arg
                continue;
            }
            value = luaL_checkinteger(L, j+1+2);
            // Changes the state globally for all mobjs of this type, quite a hack...
            // But atleast it's restored later
            if (is_misc) {
                switch (j) {
                    case 0:
                        orig_state->misc1 = value;
                        break;
                    case 1:
                        orig_state->misc2 = value;
                        break;
                    default:
                        // This isn't going to happen...
                        break;
                }
            }
            else {
                orig_state->args[j] = value;
            }
        }

        c_cptr.p1(*mobj_lua);
        // Restore state args
        if (is_misc) {
            orig_state->misc1 = orig_misc1;
            orig_state->misc2 = orig_misc2;
        }
        else {
            memcpy(orig_state->args, orig_args, sizeof(orig_args));
        }
        found = true;
    }
    else if (!is_misc) {
        // Look for Lua codepointer
        do
        {
            ++i;
            if (!strcasecmp(key, lua_cptrs[i].lookup))
            {
                CallLuaCptrP1(lua_cptrs[i].cptr, *mobj_lua);
                found = true;
            }
        } while (!found && i < lua_cptrs_count);
    }

    if (!found) {
        luaL_argerror(L, 2, "codepointer not found");
    }

    return 0;
}

static int l_mobj_call(lua_State* L) {
    return l_mobj_callGeneric(L, false);
}

// call(...) but for Misc1/Misc2 arguments (only valid for built-in codepointers)
static int l_mobj_callMisc(lua_State* L) {
    return l_mobj_callGeneric(L, true);
}

static int l_mobj_checkSight(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    mobj_t** mobj2_lua = CheckMobjInIndex(L, 2);
    boolean result = P_CheckSight(*mobj_lua, *mobj2_lua);
    lua_pushboolean(L, result);
    return 1;
}

static int l_mobj_takeDamage(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    mobj_t** inflictor_lua = CheckMobjInIndex(L, 2);
    mobj_t** source_lua = CheckMobjInIndex(L, 3);
    int damage = luaL_checkinteger(L, 4);
    P_DamageMobj(*mobj_lua, *inflictor_lua, *source_lua, damage);
    return 0;
}

static int l_mobj_radiusAttack(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    mobj_t** source_lua = CheckMobjInIndex(L, 2);
    int damage = luaL_checkinteger(L, 3);
    int distance = luaL_checkinteger(L, 4);
    P_RadiusAttack(*mobj_lua, *source_lua, damage, distance);
    return 0;
}

static int l_mobj_spawnMissile(lua_State* L) {
    mobj_t* mo;
    mobj_t** mobj_lua = CheckMobj(L);
    mobj_t** dest_lua = CheckMobjInIndex(L, 2);
    int type = luaL_checkinteger(L, 3);
    type -= 1;

    mo = P_SpawnMissile(*mobj_lua, *dest_lua, type);
    if (!mo) {
        // This shouldn't happen, I think...
        luaL_error(L, "Failed to spawn mobj");
    }

    NewMobj(L, mo);
    return 1;
}

static int l_mobj_setPos(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    P_UnsetThingPosition(*mobj_lua);
    (*mobj_lua)->x = luaL_optinteger(L, 2, (*mobj_lua)->x);
    (*mobj_lua)->y = luaL_optinteger(L, 3, (*mobj_lua)->y);
    (*mobj_lua)->z = luaL_optinteger(L, 4, (*mobj_lua)->z);
    P_SetThingPosition(*mobj_lua);
    return 0;
}

static int l_mobj_aimLineAttack(lua_State* L) {
    fixed_t slope;
    boolean skip_friends;
    mobj_t** mobj_lua = CheckMobj(L);
    angle_t angle = luaL_checkinteger(L, 2);
    fixed_t distance = luaL_optinteger(L, 3, MISSILERANGE);
    skip_friends = lua_isboolean(L, 4) ? lua_toboolean(L, 4) : false;
    slope = P_AimLineAttack(*mobj_lua, angle, distance, skip_friends ? MF_FRIEND : 0);
    lua_pushinteger(L, slope);
    return 1;
}

static int l_mobj_lineAttack(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    angle_t angle = luaL_checkinteger(L, 2);
    fixed_t distance = luaL_optinteger(L, 3, MISSILERANGE);
    fixed_t slope = luaL_checkinteger(L, 4);
    int damage = luaL_checkinteger(L, 5);
    P_LineAttack(*mobj_lua, angle, distance, slope, damage);
    return 0;
}

static int l_mobjIndex(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "health") == 0) {
        lua_pushinteger(L, (*mobj_lua)->health);
    }
    else if (strcmp(key, "spawnhealth") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->spawnhealth);
    }
    else if (strcmp(key, "target") == 0) {
        if ((*mobj_lua)->target) {
            NewMobj(L, (*mobj_lua)->target);
        }
        else {
            lua_pushnil(L);
        }
    }
    else if (strcmp(key, "tracer") == 0) {
        if ((*mobj_lua)->tracer) {
            NewMobj(L, (*mobj_lua)->tracer);
        }
        else {
            lua_pushnil(L);
        }
    }
    else if (strcmp(key, "mass") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->mass);
    }
    else if (strcmp(key, "meleerange") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->meleerange);
    }
    else if (strcmp(key, "momx") == 0) {
        lua_pushinteger(L, (*mobj_lua)->momx);
    }
    else if (strcmp(key, "momy") == 0) {
        lua_pushinteger(L, (*mobj_lua)->momy);
    }
    else if (strcmp(key, "momz") == 0) {
        lua_pushinteger(L, (*mobj_lua)->momz);
    }
    else if (strcmp(key, "angle") == 0) {
        lua_pushinteger(L, (*mobj_lua)->angle);
    }
    else if (strcmp(key, "x") == 0) {
        lua_pushinteger(L, (*mobj_lua)->x);
    }
    else if (strcmp(key, "y") == 0) {
        lua_pushinteger(L, (*mobj_lua)->y);
    }
    else if (strcmp(key, "z") == 0) {
        lua_pushinteger(L, (*mobj_lua)->z);
    }
    else {
        int v_type = luaL_getmetafield(L, 1, key);
        if (v_type == LUA_TNIL) {
            luaL_argerror(L, 2, "invalid mobj attribute");
        }
    }

    return 1;
}

static int l_mobjNewIndex(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "health") == 0) {
        int value = luaL_checkinteger(L, 3);
        (*mobj_lua)->health = value;
    }
    else if (strcmp(key, "target") == 0) {
        mobj_t** new_target_lua;
        if (lua_isnil(L, 3)) {
            P_SetTarget(&((*mobj_lua)->target), NULL);
        }
        else {
            new_target_lua = CheckMobjInIndex(L, 3);
            P_SetTarget(&((*mobj_lua)->target), *new_target_lua);
        }
    }
    else if (strcmp(key, "tracer") == 0) {
        mobj_t** new_tracer_lua;
        if (lua_isnil(L, 3)) {
            P_SetTarget(&((*mobj_lua)->tracer), NULL);
        }
        else {
            new_tracer_lua = CheckMobjInIndex(L, 3);
            P_SetTarget(&((*mobj_lua)->tracer), *new_tracer_lua);
        }
    }
    else if (strcmp(key, "momx") == 0) {
        int value = luaL_checkinteger(L, 3);
        (*mobj_lua)->momx = value;
    }
    else if (strcmp(key, "momy") == 0) {
        int value = luaL_checkinteger(L, 3);
        (*mobj_lua)->momy = value;
    }
    else if (strcmp(key, "momz") == 0) {
        int value = luaL_checkinteger(L, 3);
        (*mobj_lua)->momz = value;
    }
    else if (strcmp(key, "angle") == 0) {
        angle_t value = luaL_checkinteger(L, 3);
        (*mobj_lua)->angle = value;
    }
    else {
        luaL_argerror(L, 2, "invalid mobj attribute");
    }

    return 0;
}

static int l_mobjEq(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    mobj_t** mobj2_lua = CheckMobjInIndex(L, 2);
    lua_pushboolean(L, *mobj_lua == *mobj2_lua);
    return 1;
}

static const struct luaL_Reg mobj_lib[] = {
    {"__index", l_mobjIndex},
    {"__newindex", l_mobjNewIndex},
    {"__eq", l_mobjEq},
    {"call", l_mobj_call},
    {"callMisc", l_mobj_callMisc},
    {"checkSight", l_mobj_checkSight},
    {"takeDamage", l_mobj_takeDamage},
    {"radiusAttack", l_mobj_radiusAttack},
    {"spawnMissile", l_mobj_spawnMissile},
    {"setPos", l_mobj_setPos},
    {"aimLineAttack", l_mobj_aimLineAttack},
    {"lineAttack", l_mobj_lineAttack},
    {NULL, NULL}
};

void LoadMobjMetatable(lua_State* L) {
    luaL_newmetatable(L, MOBJ_META);

    luaL_setfuncs(L, mobj_lib, 0);
}
