#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"
#include "d_lua_mobj.h"
#include "d_lua_player.h"
#include "d_lua_pspr.h"

#include "m_io.h"
#include "m_random.h"

#include "doomdef.h"
#include "i_printf.h"
#include "w_wad.h"
#include "memio.h"
#include "i_system.h" // I_Realloc

#define LUA_BUFFER_START_SIZE 1024
#define LUA_CPTRS_MAX 1024

lua_State* L_state;

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

static void LoadLuahackFuncs() {
    lua_pushcfunction(L_state, l_registerCodepointer);
    lua_setglobal(L_state, "registerCodepointer");

    lua_pushcfunction(L_state, l_tofixed);
    lua_setglobal(L_state, "tofixed");

    lua_pushcfunction(L_state, l_fromfixed);
    lua_setglobal(L_state, "fromfixed");

    lua_pushcfunction(L_state, l_fixedToAngle);
    lua_setglobal(L_state, "fixedToAngle");

    lua_pushcfunction(L_state, l_angleToFixed);
    lua_setglobal(L_state, "angleToFixed");

    lua_pushcfunction(L_state, l_tan);
    lua_setglobal(L_state, "tan");

    lua_pushcfunction(L_state, l_sin);
    lua_setglobal(L_state, "sin");

    lua_pushcfunction(L_state, l_cos);
    lua_setglobal(L_state, "cos");

    lua_pushcfunction(L_state, l_random);
    lua_setglobal(L_state, "random");

    lua_pushcfunction(L_state, l_spawnMobj);
    lua_setglobal(L_state, "spawnMobj");
}

void CloseLua() {
    lua_close(L_state);
}

static void OpenLua() {
    L_state = luaL_newstate();
    I_AtExit(CloseLua, true);
    luaL_openlibs(L_state);
    LoadLuahackFuncs();
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

void CallLuaCptrP1(int cptr, mobj_t* mobj) {
    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    NewMobj(L_state, mobj);
    if (lua_pcall(L_state, 1, 0, 0) != 0) {
        I_Error("%s", lua_tostring(L_state, -1));
    }
}

void CallLuaCptrP2(int cptr, player_t* player, pspdef_t* psp) {
    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    NewPlayer(L_state, player);
    NewPspr(L_state, psp);
    if (lua_pcall(L_state, 2, 0, 0) != 0) {
        I_Error("%s", lua_tostring(L_state, -1));
    }
}
