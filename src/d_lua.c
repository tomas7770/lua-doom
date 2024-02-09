#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"
#include "d_lua_mobj.h"
#include "d_lua_player.h"
#include "d_lua_pspr.h"

#include "m_io.h"
#include "m_random.h"
#include "r_main.h"

#include "doomdef.h"
#include "i_printf.h"
#include "w_wad.h"
#include "memio.h"
#include "i_system.h" // I_Realloc

extern skill_t gameskill;

#define LUA_BUFFER_START_SIZE 1024
#define LUA_CPTRS_MAX 1024

lua_State* L_state = NULL;

lua_cptr lua_cptrs[LUA_CPTRS_MAX];
size_t lua_cptrs_count = 0;

static int l_registerCodepointer(lua_State* L) {
    int cptr, cptr_name_len;
    const char* cptr_name = luaL_checkstring(L, 1);

    if (lua_gettop(L) != 2) {
        luaL_error(L, "registerCodepointer: expected 2 arguments");
    }

    luaL_argexpected(L, lua_isfunction(L, 2), 2, "function");
    cptr = luaL_ref(L, LUA_REGISTRYINDEX);
    cptr_name_len = lua_rawlen(L, 1);

    if (lua_cptrs_count >= LUA_CPTRS_MAX) {
        // We have a problem
        I_Error("Reached Lua codepointer limit");
    }
    if (cptr_name_len >= LUA_CPTR_NAME_SIZE) {
        I_Error("Lua codepointer %s name exceeds maximum length", cptr_name);
    }

    strcpy(lua_cptrs[lua_cptrs_count].lookup, "A_");
    memcpy(lua_cptrs[lua_cptrs_count].lookup+sizeof("A_")-1, cptr_name, cptr_name_len);
    lua_cptrs[lua_cptrs_count].cptr = cptr;

    lua_cptrs_count++;
    return 0;
}

static void RegisterMetatableMethod(lua_State* L, const char* tname) {
    if (lua_gettop(L) != 2) {
        luaL_error(L, "expected 2 arguments");
    }
    luaL_checkstring(L, 1);
    luaL_argexpected(L, lua_isfunction(L, 2), 2, "function");

    if (luaL_getmetatable(L, tname) == LUA_TNIL) {
        luaL_error(L, "Attempt to register method on unknown metatable");
    }
    lua_pushvalue(L, 1);
    if (lua_rawget(L, -2) != LUA_TNIL) {
        luaL_argerror(L, 1, "given method name is already used");
    }
    lua_pop(L, 1);

    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawset(L, -3);
}

static int l_registerMobjMethod(lua_State* L) {
    RegisterMetatableMethod(L, MOBJ_META);
    return 0;
}

static int l_registerPlayerMethod(lua_State* L) {
    RegisterMetatableMethod(L, PLAYER_META);
    return 0;
}

static int l_tofixed(lua_State* L) {
    double x = luaL_checknumber(L, 1);
    int result = FRACUNIT*x;
    lua_pushinteger(L, result);
    return 1;
}

static int l_fromfixed(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    lua_pushnumber(L, x);
    lua_pushnumber(L, FRACUNIT);
    lua_arith(L, LUA_OPDIV);
    return 1;
}

static int l_fixedToAngle(lua_State* L) {
    fixed_t x = luaL_checkinteger(L, 1);
    int result = FixedToAngle(x);
    lua_pushinteger(L, result);
    return 1;
}

static int l_angleToFixed(lua_State* L) {
    angle_t x = luaL_checkinteger(L, 1);
    int result = AngleToFixed(x);
    lua_pushinteger(L, result);
    return 1;
}

static int l_tan(lua_State* L) {
    angle_t an = luaL_checkinteger(L, 1);
    an += ANG90;
    an >>= ANGLETOFINESHIFT;
    lua_pushinteger(L, finetangent[an % (FINEANGLES/2)]);
    return 1;
}

static int l_sin(lua_State* L) {
    angle_t an = luaL_checkinteger(L, 1);
    an >>= ANGLETOFINESHIFT;
    lua_pushinteger(L, finesine[an % FINEANGLES]);
    return 1;
}

static int l_cos(lua_State* L) {
    angle_t an = luaL_checkinteger(L, 1);
    an >>= ANGLETOFINESHIFT;
    lua_pushinteger(L, finecosine[an % FINEANGLES]);
    return 1;
}

static int l_pointToAngle(lua_State* L) {
    fixed_t x1 = luaL_checkinteger(L, 1);
    fixed_t y1 = luaL_checkinteger(L, 2);
    fixed_t x2 = luaL_checkinteger(L, 3);
    fixed_t y2 = luaL_checkinteger(L, 4);
    lua_pushinteger(L, R_PointToAngle2(x1, y1, x2, y2));
    return 1;
}

static int l_random(lua_State* L) {
    lua_pushinteger(L, P_Random(pr_mbf21));
    return 1;
}

static int l_spawnMobj(lua_State* L) {
    mobj_t* mo;
    int type = luaL_checkinteger(L, 1);
    fixed_t x = luaL_checkinteger(L, 2);
    fixed_t y = luaL_checkinteger(L, 3);
    fixed_t z = luaL_checkinteger(L, 4);
    type -= 1;

    mo = P_SpawnMobj(x, y, z, type);
    if (!mo) {
        // This shouldn't happen, I think...
        luaL_error(L, "Failed to spawn mobj");
    }

    NewMobj(L, mo);
    return 1;
}

static int l_getGameSkill(lua_State* L) {
    lua_pushinteger(L, gameskill);
    return 1;
}

static void LoadLuahackFuncs(lua_State* L) {
    lua_pushcfunction(L, l_registerCodepointer);
    lua_setglobal(L, "registerCodepointer");

    lua_pushcfunction(L, l_registerMobjMethod);
    lua_setglobal(L, "registerMobjMethod");

    lua_pushcfunction(L, l_registerPlayerMethod);
    lua_setglobal(L, "registerPlayerMethod");

    lua_pushcfunction(L, l_tofixed);
    lua_setglobal(L, "tofixed");

    lua_pushcfunction(L, l_fromfixed);
    lua_setglobal(L, "fromfixed");

    lua_pushcfunction(L, l_fixedToAngle);
    lua_setglobal(L, "fixedToAngle");

    lua_pushcfunction(L, l_angleToFixed);
    lua_setglobal(L, "angleToFixed");

    lua_pushcfunction(L, l_tan);
    lua_setglobal(L, "tan");

    lua_pushcfunction(L, l_sin);
    lua_setglobal(L, "sin");

    lua_pushcfunction(L, l_cos);
    lua_setglobal(L, "cos");

    lua_pushcfunction(L, l_pointToAngle);
    lua_setglobal(L, "pointToAngle");

    lua_pushcfunction(L, l_random);
    lua_setglobal(L, "random");

    lua_pushcfunction(L, l_spawnMobj);
    lua_setglobal(L, "spawnMobj");

    lua_pushcfunction(L, l_getGameSkill);
    lua_setglobal(L, "getGameSkill");
}

static void LoadLuahackConsts(lua_State* L) {
    // Cheats
    lua_pushinteger(L, CF_NOCLIP); lua_setglobal(L, "CHEATS_NOCLIP");
    lua_pushinteger(L, CF_GODMODE); lua_setglobal(L, "CHEATS_GOD");
    lua_pushinteger(L, CF_NOMOMENTUM); lua_setglobal(L, "CHEATS_NOMOMENTUM");
    lua_pushinteger(L, CF_BUDDHA); lua_setglobal(L, "CHEATS_BUDDHA");
    lua_pushinteger(L, CF_NOTARGET); lua_setglobal(L, "CHEATS_NOTARGET");
    // Powers
    lua_pushinteger(L, pw_invulnerability); lua_setglobal(L, "POWERS_INVUL");
    lua_pushinteger(L, pw_strength); lua_setglobal(L, "POWERS_BERSERK");
    lua_pushinteger(L, pw_invisibility); lua_setglobal(L, "POWERS_INVIS");
    lua_pushinteger(L, pw_ironfeet); lua_setglobal(L, "POWERS_RAD");
    lua_pushinteger(L, pw_allmap); lua_setglobal(L, "POWERS_MAP");
    lua_pushinteger(L, pw_infrared); lua_setglobal(L, "POWERS_LIGHT");
    // Skill levels
    lua_pushinteger(L, sk_baby); lua_setglobal(L, "SK_BABY");
    lua_pushinteger(L, sk_easy); lua_setglobal(L, "SK_EASY");
    lua_pushinteger(L, sk_medium); lua_setglobal(L, "SK_MEDIUM");
    lua_pushinteger(L, sk_hard); lua_setglobal(L, "SK_HARD");
    lua_pushinteger(L, sk_nightmare); lua_setglobal(L, "SK_NIGHTMARE");
    // Mobj flags
    lua_pushinteger(L, MF_SPECIAL); lua_setglobal(L, "MF_SPECIAL");
    lua_pushinteger(L, MF_SOLID); lua_setglobal(L, "MF_SOLID");
    lua_pushinteger(L, MF_SHOOTABLE); lua_setglobal(L, "MF_SHOOTABLE");
    lua_pushinteger(L, MF_NOSECTOR); lua_setglobal(L, "MF_NOSECTOR");
    lua_pushinteger(L, MF_NOBLOCKMAP); lua_setglobal(L, "MF_NOBLOCKMAP");
    lua_pushinteger(L, MF_AMBUSH); lua_setglobal(L, "MF_AMBUSH");
    lua_pushinteger(L, MF_JUSTHIT); lua_setglobal(L, "MF_JUSTHIT");
    lua_pushinteger(L, MF_JUSTATTACKED); lua_setglobal(L, "MF_JUSTATTACKED");
    lua_pushinteger(L, MF_SPAWNCEILING); lua_setglobal(L, "MF_SPAWNCEILING");
    lua_pushinteger(L, MF_NOGRAVITY); lua_setglobal(L, "MF_NOGRAVITY");
    lua_pushinteger(L, MF_DROPOFF); lua_setglobal(L, "MF_DROPOFF");
    lua_pushinteger(L, MF_PICKUP); lua_setglobal(L, "MF_PICKUP");
    lua_pushinteger(L, MF_NOCLIP); lua_setglobal(L, "MF_NOCLIP");
    lua_pushinteger(L, MF_SLIDE); lua_setglobal(L, "MF_SLIDE");
    lua_pushinteger(L, MF_FLOAT); lua_setglobal(L, "MF_FLOAT");
    lua_pushinteger(L, MF_TELEPORT); lua_setglobal(L, "MF_TELEPORT");
    lua_pushinteger(L, MF_MISSILE); lua_setglobal(L, "MF_MISSILE");
    lua_pushinteger(L, MF_DROPPED); lua_setglobal(L, "MF_DROPPED");
    lua_pushinteger(L, MF_SHADOW); lua_setglobal(L, "MF_SHADOW");
    lua_pushinteger(L, MF_NOBLOOD); lua_setglobal(L, "MF_NOBLOOD");
    lua_pushinteger(L, MF_CORPSE); lua_setglobal(L, "MF_CORPSE");
    lua_pushinteger(L, MF_INFLOAT); lua_setglobal(L, "MF_INFLOAT");
    lua_pushinteger(L, MF_COUNTKILL); lua_setglobal(L, "MF_COUNTKILL");
    lua_pushinteger(L, MF_COUNTITEM); lua_setglobal(L, "MF_COUNTITEM");
    lua_pushinteger(L, MF_SKULLFLY); lua_setglobal(L, "MF_SKULLFLY");
    lua_pushinteger(L, MF_NOTDMATCH); lua_setglobal(L, "MF_NOTDMATCH");
    lua_pushinteger(L, MF_TRANSLATION); lua_setglobal(L, "MF_TRANSLATION");
    lua_pushinteger(L, MF_TRANSSHIFT); lua_setglobal(L, "MF_TRANSSHIFT");
    lua_pushinteger(L, MF_TOUCHY); lua_setglobal(L, "MF_TOUCHY");
    lua_pushinteger(L, MF_BOUNCES); lua_setglobal(L, "MF_BOUNCES");
    lua_pushinteger(L, MF_FRIEND); lua_setglobal(L, "MF_FRIEND");
    lua_pushinteger(L, MF_TRANSLUCENT); lua_setglobal(L, "MF_TRANSLUCENT");
    lua_pushinteger(L, MF2_LOGRAV); lua_setglobal(L, "MF2_LOGRAV");
    lua_pushinteger(L, MF2_SHORTMRANGE); lua_setglobal(L, "MF2_SHORTMRANGE");
    lua_pushinteger(L, MF2_DMGIGNORED); lua_setglobal(L, "MF2_DMGIGNORED");
    lua_pushinteger(L, MF2_NORADIUSDMG); lua_setglobal(L, "MF2_NORADIUSDMG");
    lua_pushinteger(L, MF2_FORCERADIUSDMG); lua_setglobal(L, "MF2_FORCERADIUSDMG");
    lua_pushinteger(L, MF2_HIGHERMPROB); lua_setglobal(L, "MF2_HIGHERMPROB");
    lua_pushinteger(L, MF2_RANGEHALF); lua_setglobal(L, "MF2_RANGEHALF");
    lua_pushinteger(L, MF2_NOTHRESHOLD); lua_setglobal(L, "MF2_NOTHRESHOLD");
    lua_pushinteger(L, MF2_LONGMELEE); lua_setglobal(L, "MF2_LONGMELEE");
    lua_pushinteger(L, MF2_BOSS); lua_setglobal(L, "MF2_BOSS");
    lua_pushinteger(L, MF2_MAP07BOSS1); lua_setglobal(L, "MF2_MAP07BOSS1");
    lua_pushinteger(L, MF2_MAP07BOSS2); lua_setglobal(L, "MF2_MAP07BOSS2");
    lua_pushinteger(L, MF2_E1M8BOSS); lua_setglobal(L, "MF2_E1M8BOSS");
    lua_pushinteger(L, MF2_E2M8BOSS); lua_setglobal(L, "MF2_E2M8BOSS");
    lua_pushinteger(L, MF2_E3M8BOSS); lua_setglobal(L, "MF2_E3M8BOSS");
    lua_pushinteger(L, MF2_E4M6BOSS); lua_setglobal(L, "MF2_E4M6BOSS");
    lua_pushinteger(L, MF2_E4M8BOSS); lua_setglobal(L, "MF2_E4M8BOSS");
    lua_pushinteger(L, MF2_RIP); lua_setglobal(L, "MF2_RIP");
    lua_pushinteger(L, MF2_FULLVOLSOUNDS); lua_setglobal(L, "MF2_FULLVOLSOUNDS");
    // Mobj types
    lua_pushinteger(L, MT_PLAYER+1); lua_setglobal(L, "MT_PLAYER");
    lua_pushinteger(L, MT_POSSESSED+1); lua_setglobal(L, "MT_POSSESSED");
    lua_pushinteger(L, MT_SHOTGUY+1); lua_setglobal(L, "MT_SHOTGUY");
    lua_pushinteger(L, MT_VILE+1); lua_setglobal(L, "MT_VILE");
    lua_pushinteger(L, MT_FIRE+1); lua_setglobal(L, "MT_FIRE");
    lua_pushinteger(L, MT_UNDEAD+1); lua_setglobal(L, "MT_UNDEAD");
    lua_pushinteger(L, MT_TRACER+1); lua_setglobal(L, "MT_TRACER");
    lua_pushinteger(L, MT_SMOKE+1); lua_setglobal(L, "MT_SMOKE");
    lua_pushinteger(L, MT_FATSO+1); lua_setglobal(L, "MT_FATSO");
    lua_pushinteger(L, MT_FATSHOT+1); lua_setglobal(L, "MT_FATSHOT");
    lua_pushinteger(L, MT_CHAINGUY+1); lua_setglobal(L, "MT_CHAINGUY");
    lua_pushinteger(L, MT_TROOP+1); lua_setglobal(L, "MT_TROOP");
    lua_pushinteger(L, MT_SERGEANT+1); lua_setglobal(L, "MT_SERGEANT");
    lua_pushinteger(L, MT_SHADOWS+1); lua_setglobal(L, "MT_SHADOWS");
    lua_pushinteger(L, MT_HEAD+1); lua_setglobal(L, "MT_HEAD");
    lua_pushinteger(L, MT_BRUISER+1); lua_setglobal(L, "MT_BRUISER");
    lua_pushinteger(L, MT_BRUISERSHOT+1); lua_setglobal(L, "MT_BRUISERSHOT");
    lua_pushinteger(L, MT_KNIGHT+1); lua_setglobal(L, "MT_KNIGHT");
    lua_pushinteger(L, MT_SKULL+1); lua_setglobal(L, "MT_SKULL");
    lua_pushinteger(L, MT_SPIDER+1); lua_setglobal(L, "MT_SPIDER");
    lua_pushinteger(L, MT_BABY+1); lua_setglobal(L, "MT_BABY");
    lua_pushinteger(L, MT_CYBORG+1); lua_setglobal(L, "MT_CYBORG");
    lua_pushinteger(L, MT_PAIN+1); lua_setglobal(L, "MT_PAIN");
    lua_pushinteger(L, MT_WOLFSS+1); lua_setglobal(L, "MT_WOLFSS");
    lua_pushinteger(L, MT_KEEN+1); lua_setglobal(L, "MT_KEEN");
    lua_pushinteger(L, MT_BOSSBRAIN+1); lua_setglobal(L, "MT_BOSSBRAIN");
    lua_pushinteger(L, MT_BOSSSPIT+1); lua_setglobal(L, "MT_BOSSSPIT");
    lua_pushinteger(L, MT_BOSSTARGET+1); lua_setglobal(L, "MT_BOSSTARGET");
    lua_pushinteger(L, MT_SPAWNSHOT+1); lua_setglobal(L, "MT_SPAWNSHOT");
    lua_pushinteger(L, MT_SPAWNFIRE+1); lua_setglobal(L, "MT_SPAWNFIRE");
    lua_pushinteger(L, MT_BARREL+1); lua_setglobal(L, "MT_BARREL");
    lua_pushinteger(L, MT_TROOPSHOT+1); lua_setglobal(L, "MT_TROOPSHOT");
    lua_pushinteger(L, MT_HEADSHOT+1); lua_setglobal(L, "MT_HEADSHOT");
    lua_pushinteger(L, MT_ROCKET+1); lua_setglobal(L, "MT_ROCKET");
    lua_pushinteger(L, MT_PLASMA+1); lua_setglobal(L, "MT_PLASMA");
    lua_pushinteger(L, MT_BFG+1); lua_setglobal(L, "MT_BFG");
    lua_pushinteger(L, MT_ARACHPLAZ+1); lua_setglobal(L, "MT_ARACHPLAZ");
    lua_pushinteger(L, MT_PUFF+1); lua_setglobal(L, "MT_PUFF");
    lua_pushinteger(L, MT_BLOOD+1); lua_setglobal(L, "MT_BLOOD");
    lua_pushinteger(L, MT_TFOG+1); lua_setglobal(L, "MT_TFOG");
    lua_pushinteger(L, MT_IFOG+1); lua_setglobal(L, "MT_IFOG");
    lua_pushinteger(L, MT_TELEPORTMAN+1); lua_setglobal(L, "MT_TELEPORTMAN");
    lua_pushinteger(L, MT_EXTRABFG+1); lua_setglobal(L, "MT_EXTRABFG");
    lua_pushinteger(L, MT_MISC0+1); lua_setglobal(L, "MT_MISC0");
    lua_pushinteger(L, MT_MISC1+1); lua_setglobal(L, "MT_MISC1");
    lua_pushinteger(L, MT_MISC2+1); lua_setglobal(L, "MT_MISC2");
    lua_pushinteger(L, MT_MISC3+1); lua_setglobal(L, "MT_MISC3");
    lua_pushinteger(L, MT_MISC4+1); lua_setglobal(L, "MT_MISC4");
    lua_pushinteger(L, MT_MISC5+1); lua_setglobal(L, "MT_MISC5");
    lua_pushinteger(L, MT_MISC6+1); lua_setglobal(L, "MT_MISC6");
    lua_pushinteger(L, MT_MISC7+1); lua_setglobal(L, "MT_MISC7");
    lua_pushinteger(L, MT_MISC8+1); lua_setglobal(L, "MT_MISC8");
    lua_pushinteger(L, MT_MISC9+1); lua_setglobal(L, "MT_MISC9");
    lua_pushinteger(L, MT_MISC10+1); lua_setglobal(L, "MT_MISC10");
    lua_pushinteger(L, MT_MISC11+1); lua_setglobal(L, "MT_MISC11");
    lua_pushinteger(L, MT_MISC12+1); lua_setglobal(L, "MT_MISC12");
    lua_pushinteger(L, MT_INV+1); lua_setglobal(L, "MT_INV");
    lua_pushinteger(L, MT_MISC13+1); lua_setglobal(L, "MT_MISC13");
    lua_pushinteger(L, MT_INS+1); lua_setglobal(L, "MT_INS");
    lua_pushinteger(L, MT_MISC14+1); lua_setglobal(L, "MT_MISC14");
    lua_pushinteger(L, MT_MISC15+1); lua_setglobal(L, "MT_MISC15");
    lua_pushinteger(L, MT_MISC16+1); lua_setglobal(L, "MT_MISC16");
    lua_pushinteger(L, MT_MEGA+1); lua_setglobal(L, "MT_MEGA");
    lua_pushinteger(L, MT_CLIP+1); lua_setglobal(L, "MT_CLIP");
    lua_pushinteger(L, MT_MISC17+1); lua_setglobal(L, "MT_MISC17");
    lua_pushinteger(L, MT_MISC18+1); lua_setglobal(L, "MT_MISC18");
    lua_pushinteger(L, MT_MISC19+1); lua_setglobal(L, "MT_MISC19");
    lua_pushinteger(L, MT_MISC20+1); lua_setglobal(L, "MT_MISC20");
    lua_pushinteger(L, MT_MISC21+1); lua_setglobal(L, "MT_MISC21");
    lua_pushinteger(L, MT_MISC22+1); lua_setglobal(L, "MT_MISC22");
    lua_pushinteger(L, MT_MISC23+1); lua_setglobal(L, "MT_MISC23");
    lua_pushinteger(L, MT_MISC24+1); lua_setglobal(L, "MT_MISC24");
    lua_pushinteger(L, MT_MISC25+1); lua_setglobal(L, "MT_MISC25");
    lua_pushinteger(L, MT_CHAINGUN+1); lua_setglobal(L, "MT_CHAINGUN");
    lua_pushinteger(L, MT_MISC26+1); lua_setglobal(L, "MT_MISC26");
    lua_pushinteger(L, MT_MISC27+1); lua_setglobal(L, "MT_MISC27");
    lua_pushinteger(L, MT_MISC28+1); lua_setglobal(L, "MT_MISC28");
    lua_pushinteger(L, MT_SHOTGUN+1); lua_setglobal(L, "MT_SHOTGUN");
    lua_pushinteger(L, MT_SUPERSHOTGUN+1); lua_setglobal(L, "MT_SUPERSHOTGUN");
    lua_pushinteger(L, MT_MISC29+1); lua_setglobal(L, "MT_MISC29");
    lua_pushinteger(L, MT_MISC30+1); lua_setglobal(L, "MT_MISC30");
    lua_pushinteger(L, MT_MISC31+1); lua_setglobal(L, "MT_MISC31");
    lua_pushinteger(L, MT_MISC32+1); lua_setglobal(L, "MT_MISC32");
    lua_pushinteger(L, MT_MISC33+1); lua_setglobal(L, "MT_MISC33");
    lua_pushinteger(L, MT_MISC34+1); lua_setglobal(L, "MT_MISC34");
    lua_pushinteger(L, MT_MISC35+1); lua_setglobal(L, "MT_MISC35");
    lua_pushinteger(L, MT_MISC36+1); lua_setglobal(L, "MT_MISC36");
    lua_pushinteger(L, MT_MISC37+1); lua_setglobal(L, "MT_MISC37");
    lua_pushinteger(L, MT_MISC38+1); lua_setglobal(L, "MT_MISC38");
    lua_pushinteger(L, MT_MISC39+1); lua_setglobal(L, "MT_MISC39");
    lua_pushinteger(L, MT_MISC40+1); lua_setglobal(L, "MT_MISC40");
    lua_pushinteger(L, MT_MISC41+1); lua_setglobal(L, "MT_MISC41");
    lua_pushinteger(L, MT_MISC42+1); lua_setglobal(L, "MT_MISC42");
    lua_pushinteger(L, MT_MISC43+1); lua_setglobal(L, "MT_MISC43");
    lua_pushinteger(L, MT_MISC44+1); lua_setglobal(L, "MT_MISC44");
    lua_pushinteger(L, MT_MISC45+1); lua_setglobal(L, "MT_MISC45");
    lua_pushinteger(L, MT_MISC46+1); lua_setglobal(L, "MT_MISC46");
    lua_pushinteger(L, MT_MISC47+1); lua_setglobal(L, "MT_MISC47");
    lua_pushinteger(L, MT_MISC48+1); lua_setglobal(L, "MT_MISC48");
    lua_pushinteger(L, MT_MISC49+1); lua_setglobal(L, "MT_MISC49");
    lua_pushinteger(L, MT_MISC50+1); lua_setglobal(L, "MT_MISC50");
    lua_pushinteger(L, MT_MISC51+1); lua_setglobal(L, "MT_MISC51");
    lua_pushinteger(L, MT_MISC52+1); lua_setglobal(L, "MT_MISC52");
    lua_pushinteger(L, MT_MISC53+1); lua_setglobal(L, "MT_MISC53");
    lua_pushinteger(L, MT_MISC54+1); lua_setglobal(L, "MT_MISC54");
    lua_pushinteger(L, MT_MISC55+1); lua_setglobal(L, "MT_MISC55");
    lua_pushinteger(L, MT_MISC56+1); lua_setglobal(L, "MT_MISC56");
    lua_pushinteger(L, MT_MISC57+1); lua_setglobal(L, "MT_MISC57");
    lua_pushinteger(L, MT_MISC58+1); lua_setglobal(L, "MT_MISC58");
    lua_pushinteger(L, MT_MISC59+1); lua_setglobal(L, "MT_MISC59");
    lua_pushinteger(L, MT_MISC60+1); lua_setglobal(L, "MT_MISC60");
    lua_pushinteger(L, MT_MISC61+1); lua_setglobal(L, "MT_MISC61");
    lua_pushinteger(L, MT_MISC62+1); lua_setglobal(L, "MT_MISC62");
    lua_pushinteger(L, MT_MISC63+1); lua_setglobal(L, "MT_MISC63");
    lua_pushinteger(L, MT_MISC64+1); lua_setglobal(L, "MT_MISC64");
    lua_pushinteger(L, MT_MISC65+1); lua_setglobal(L, "MT_MISC65");
    lua_pushinteger(L, MT_MISC66+1); lua_setglobal(L, "MT_MISC66");
    lua_pushinteger(L, MT_MISC67+1); lua_setglobal(L, "MT_MISC67");
    lua_pushinteger(L, MT_MISC68+1); lua_setglobal(L, "MT_MISC68");
    lua_pushinteger(L, MT_MISC69+1); lua_setglobal(L, "MT_MISC69");
    lua_pushinteger(L, MT_MISC70+1); lua_setglobal(L, "MT_MISC70");
    lua_pushinteger(L, MT_MISC71+1); lua_setglobal(L, "MT_MISC71");
    lua_pushinteger(L, MT_MISC72+1); lua_setglobal(L, "MT_MISC72");
    lua_pushinteger(L, MT_MISC73+1); lua_setglobal(L, "MT_MISC73");
    lua_pushinteger(L, MT_MISC74+1); lua_setglobal(L, "MT_MISC74");
    lua_pushinteger(L, MT_MISC75+1); lua_setglobal(L, "MT_MISC75");
    lua_pushinteger(L, MT_MISC76+1); lua_setglobal(L, "MT_MISC76");
    lua_pushinteger(L, MT_MISC77+1); lua_setglobal(L, "MT_MISC77");
    lua_pushinteger(L, MT_MISC78+1); lua_setglobal(L, "MT_MISC78");
    lua_pushinteger(L, MT_MISC79+1); lua_setglobal(L, "MT_MISC79");
    lua_pushinteger(L, MT_MISC80+1); lua_setglobal(L, "MT_MISC80");
    lua_pushinteger(L, MT_MISC81+1); lua_setglobal(L, "MT_MISC81");
    lua_pushinteger(L, MT_MISC82+1); lua_setglobal(L, "MT_MISC82");
    lua_pushinteger(L, MT_MISC83+1); lua_setglobal(L, "MT_MISC83");
    lua_pushinteger(L, MT_MISC84+1); lua_setglobal(L, "MT_MISC84");
    lua_pushinteger(L, MT_MISC85+1); lua_setglobal(L, "MT_MISC85");
    lua_pushinteger(L, MT_MISC86+1); lua_setglobal(L, "MT_MISC86");
    lua_pushinteger(L, MT_PUSH+1); lua_setglobal(L, "MT_PUSH");
    lua_pushinteger(L, MT_PULL+1); lua_setglobal(L, "MT_PULL");
    lua_pushinteger(L, MT_DOGS+1); lua_setglobal(L, "MT_DOGS");
    lua_pushinteger(L, MT_PLASMA1+1); lua_setglobal(L, "MT_PLASMA1");
    lua_pushinteger(L, MT_PLASMA2+1); lua_setglobal(L, "MT_PLASMA2");
    lua_pushinteger(L, MT_SCEPTRE+1); lua_setglobal(L, "MT_SCEPTRE");
    lua_pushinteger(L, MT_BIBLE+1); lua_setglobal(L, "MT_BIBLE");
    lua_pushinteger(L, MT_MUSICSOURCE+1); lua_setglobal(L, "MT_MUSICSOURCE");
}

void CloseLua() {
    lua_close(L_state);
}

static void OpenLua() {
    if (L_state) {
        return;
    }
    L_state = luaL_newstate();
    I_AtExit(CloseLua, true);
    luaL_openlibs(L_state);
    LoadLuahackFuncs(L_state);
    LoadLuahackConsts(L_state);
    LoadMobjMetatable(L_state);
    LoadPlayerMetatable(L_state);
    LoadPsprMetatable(L_state);
}

void ProcessLuaLump(int lumpnum)
{
    MEMFILE* lump;
    int n;
    int error;
    char* inbuffer = (char*) malloc(sizeof(char)*LUA_BUFFER_START_SIZE);
    ssize_t lua_buffer_size = sizeof(char)*LUA_BUFFER_START_SIZE;
    int offset = 0;

    {
        void *buf = W_CacheLumpNum(lumpnum, PU_STATIC);

        lump = mem_fopen_read(buf, W_LumpLength(lumpnum));
    }

    OpenLua();

    // loop until end of file
    while ((n = mem_fread(inbuffer+offset, sizeof(char), LUA_BUFFER_START_SIZE, lump)))
    {
        offset += sizeof(char)*n;
        if (offset >= lua_buffer_size) {
            lua_buffer_size *= 2;
            inbuffer = I_Realloc(inbuffer, lua_buffer_size);
        }
    }
    inbuffer[offset] = '\0';
    error = luaL_loadbuffer(L_state, inbuffer, strlen(inbuffer), "LUAHACK") ||
            lua_pcall(L_state, 0, 0, 0);
    if (error) {
        I_Error("%s", lua_tostring(L_state, -1));
    }

    mem_fclose(lump);
    free(inbuffer);
}

void CallLuaCptrP1(int cptr, mobj_t* mobj, long args[]) {
    int j = 0;
    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    NewMobj(L_state, mobj);
    for (j = 0; j < MAXSTATEARGS; j++) {
        lua_pushinteger(L_state, args[j]);
    }
    if (lua_pcall(L_state, 1 + MAXSTATEARGS, 0, 0) != 0) {
        I_Error("%s", lua_tostring(L_state, -1));
    }
}

void CallLuaCptrP2(int cptr, player_t* player, pspdef_t* psp, long args[]) {
    int j = 0;
    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    NewPlayer(L_state, player);
    NewPspr(L_state, psp);
    for (j = 0; j < MAXSTATEARGS; j++) {
        lua_pushinteger(L_state, args[j]);
    }
    if (lua_pcall(L_state, 2 + MAXSTATEARGS, 0, 0) != 0) {
        I_Error("%s", lua_tostring(L_state, -1));
    }
}
