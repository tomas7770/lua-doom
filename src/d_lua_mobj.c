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
    int j;
    mobj_t** mobj_lua = CheckMobj(L);
    const char* cptr_name = luaL_checkstring(L, 2);
    boolean found = false;
    int i = -1; // incremented to start at zero at the top of the loop
    int extra_args = lua_gettop(L)-2;
    
    strcpy(key,"A_");  // reusing the key area to prefix the mnemonic
    strcat(key, ptr_lstrip((char*) cptr_name));
    
    c_cptr = FindDehCodepointer(key);
    if (c_cptr.p1) {
        // Count args and apply them
        long orig_args[MAXSTATEARGS];
        long orig_misc1, orig_misc2;
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
        long args[MAXSTATEARGS] = {0};
        for (j = 0; j < extra_args && j < MAXSTATEARGS; j++) {
            args[j] = luaL_optinteger(L, j+1+2, 0);
        }
        do
        {
            ++i;
            if (!strcasecmp(key, lua_cptrs[i].lookup))
            {
                CallLuaCptrP1(lua_cptrs[i].cptr, *mobj_lua, args);
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
    if (!lua_isnoneornil(L, 2)) {
        (*mobj_lua)->x = ToFixed(luaL_checknumber(L, 2));
    }
    if (!lua_isnoneornil(L, 3)) {
        (*mobj_lua)->y = ToFixed(luaL_checknumber(L, 3));
    }
    if (!lua_isnoneornil(L, 4)) {
        (*mobj_lua)->z = ToFixed(luaL_checknumber(L, 4));
    }
    P_SetThingPosition(*mobj_lua);
    return 0;
}

static int l_mobj_aimLineAttack(lua_State* L) {
    fixed_t slope;
    boolean skip_friends;
    mobj_t** mobj_lua = CheckMobj(L);
    angle_t angle = FixedToAngle(ToFixed(luaL_checknumber(L, 2)));
    fixed_t distance = MISSILERANGE;
    if (!lua_isnoneornil(L, 3)) {
        distance = ToFixed(luaL_checknumber(L, 3));
    }
    skip_friends = lua_isboolean(L, 4) ? lua_toboolean(L, 4) : false;
    slope = P_AimLineAttack(*mobj_lua, angle, distance, skip_friends ? MF_FRIEND : 0);
    lua_pushnumber(L, FromFixed(slope));
    return 1;
}

static int l_mobj_lineAttack(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    angle_t angle = FixedToAngle(ToFixed(luaL_checknumber(L, 2)));
    fixed_t distance = MISSILERANGE;
    fixed_t slope = ToFixed(luaL_checknumber(L, 4));
    int damage = luaL_checkinteger(L, 5);
    if (!lua_isnoneornil(L, 3)) {
        distance = ToFixed(luaL_checknumber(L, 3));
    }
    P_LineAttack(*mobj_lua, angle, distance, slope, damage);
    return 0;
}

static int l_mobj_setState(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);
    statenum_t state = luaL_checkinteger(L, 2);
    P_SetMobjState(*mobj_lua, state);
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
        lua_pushnumber(L, FromFixed((*mobj_lua)->info->meleerange));
    }
    else if (strcmp(key, "radius") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->info->radius));
    }
    else if (strcmp(key, "height") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->info->height));
    }
    else if (strcmp(key, "momx") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->momx));
    }
    else if (strcmp(key, "momy") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->momy));
    }
    else if (strcmp(key, "momz") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->momz));
    }
    else if (strcmp(key, "angle") == 0) {
        lua_pushnumber(L, FromFixed(AngleToFixed((*mobj_lua)->angle)));
    }
    else if (strcmp(key, "x") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->x));
    }
    else if (strcmp(key, "y") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->y));
    }
    else if (strcmp(key, "z") == 0) {
        lua_pushnumber(L, FromFixed((*mobj_lua)->z));
    }
    else if (strcmp(key, "flags") == 0) {
        lua_pushinteger(L, (*mobj_lua)->flags);
    }
    else if (strcmp(key, "flags2") == 0) {
        lua_pushinteger(L, (*mobj_lua)->flags2);
    }
    else if (strcmp(key, "type") == 0) {
        lua_pushinteger(L, (*mobj_lua)->type+1);
    }
    else if (strcmp(key, "movedir") == 0) {
        lua_pushinteger(L, (*mobj_lua)->movedir);
    }
    else if (strcmp(key, "movecount") == 0) {
        lua_pushinteger(L, (*mobj_lua)->movecount);
    }
    else if (strcmp(key, "strafecount") == 0) {
        lua_pushinteger(L, (*mobj_lua)->strafecount);
    }
    else if (strcmp(key, "reactiontime") == 0) {
        lua_pushinteger(L, (*mobj_lua)->reactiontime);
    }
    else if (strcmp(key, "threshold") == 0) {
        lua_pushinteger(L, (*mobj_lua)->threshold);
    }
    else if (strcmp(key, "pursuecount") == 0) {
        lua_pushinteger(L, (*mobj_lua)->pursuecount);
    }
    else if (strcmp(key, "spawnstate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->spawnstate);
    }
    else if (strcmp(key, "seestate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->seestate);
    }
    else if (strcmp(key, "seesound") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->seesound);
    }
    else if (strcmp(key, "spawnreactiontime") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->reactiontime);
    }
    else if (strcmp(key, "attacksound") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->attacksound);
    }
    else if (strcmp(key, "painstate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->painstate);
    }
    else if (strcmp(key, "painchance") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->painchance);
    }
    else if (strcmp(key, "painsound") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->painsound);
    }
    else if (strcmp(key, "meleestate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->meleestate);
    }
    else if (strcmp(key, "missilestate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->missilestate);
    }
    else if (strcmp(key, "deathstate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->deathstate);
    }
    else if (strcmp(key, "xdeathstate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->xdeathstate);
    }
    else if (strcmp(key, "deathsound") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->deathsound);
    }
    else if (strcmp(key, "speed") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->speed);
    }
    else if (strcmp(key, "damage") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->damage);
    }
    else if (strcmp(key, "activesound") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->activesound);
    }
    else if (strcmp(key, "raisestate") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->raisestate);
    }
    else if (strcmp(key, "infighting_group") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->infighting_group);
    }
    else if (strcmp(key, "projectile_group") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->projectile_group);
    }
    else if (strcmp(key, "splash_group") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->splash_group);
    }
    else if (strcmp(key, "ripsound") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->ripsound);
    }
    else if (strcmp(key, "altspeed") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->altspeed);
    }
    else if (strcmp(key, "droppeditem") == 0) {
        lua_pushinteger(L, (*mobj_lua)->info->droppeditem+1);
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
        fixed_t value = ToFixed(luaL_checknumber(L, 3));
        (*mobj_lua)->momx = value;
    }
    else if (strcmp(key, "momy") == 0) {
        fixed_t value = ToFixed(luaL_checknumber(L, 3));
        (*mobj_lua)->momy = value;
    }
    else if (strcmp(key, "momz") == 0) {
        fixed_t value = ToFixed(luaL_checknumber(L, 3));
        (*mobj_lua)->momz = value;
    }
    else if (strcmp(key, "angle") == 0) {
        angle_t value = FixedToAngle(ToFixed(luaL_checknumber(L, 3)));
        (*mobj_lua)->angle = value;
    }
    else if (strcmp(key, "flags") == 0) {
        int value = luaL_checkinteger(L, 3);
        (*mobj_lua)->flags = value;
    }
    else if (strcmp(key, "flags2") == 0) {
        int value = luaL_checkinteger(L, 3);
        (*mobj_lua)->flags2 = value;
    }
    else if (strcmp(key, "movecount") == 0) {
        short value = luaL_checkinteger(L, 3);
        (*mobj_lua)->movecount = value;
    }
    else if (strcmp(key, "strafecount") == 0) {
        short value = luaL_checkinteger(L, 3);
        (*mobj_lua)->strafecount = value;
    }
    else if (strcmp(key, "reactiontime") == 0) {
        short value = luaL_checkinteger(L, 3);
        (*mobj_lua)->reactiontime = value;
    }
    else if (strcmp(key, "threshold") == 0) {
        short value = luaL_checkinteger(L, 3);
        (*mobj_lua)->threshold = value;
    }
    else if (strcmp(key, "pursuecount") == 0) {
        short value = luaL_checkinteger(L, 3);
        (*mobj_lua)->pursuecount = value;
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
    {"setState", l_mobj_setState},
    {NULL, NULL}
};

void LoadMobjMetatable(lua_State* L) {
    luaL_newmetatable(L, MOBJ_META);

    luaL_setfuncs(L, mobj_lib, 0);
}
