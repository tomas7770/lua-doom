#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "d_lua.h"

#include "m_io.h"

#include "doomdef.h"
#include "i_printf.h"
#include "w_wad.h"
#include "memio.h"
#include "i_system.h" // I_Realloc

#define LUA_BUFFER_START_SIZE 1024
#define LUA_CPTRS_MAX 1024

#define MOBJ_META "LuaDoom.Mobj"

lua_State* L_state;

lua_cptr lua_cptrs[LUA_CPTRS_MAX];
size_t lua_cptrs_count = 0;

static int l_registerCodepointer(lua_State* L) {
    const char* cptr_name = luaL_checkstring(L, 1);
    int cptr = luaL_ref(L, LUA_REGISTRYINDEX);
    int cptr_name_len = lua_rawlen(L, 1);

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

static mobj_t** CheckMobj(lua_State* L) {
    void *ud = luaL_checkudata(L, 1, MOBJ_META);
    luaL_argcheck(L, ud != NULL, 1, "`mobj' expected");
    return (mobj_t**) ud;
}

static int l_mobjIndex(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "health") == 0) {
        lua_pushnumber(L, (*mobj_lua)->health);
    }
    else {
        luaL_argerror(L, 2, "invalid mobj attribute");
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
    else {
        luaL_argerror(L, 2, "invalid mobj attribute");
    }

    return 0;
}

static void LoadLuahackFuncs() {
    lua_pushcfunction(L_state, l_registerCodepointer);
    lua_setglobal(L_state, "registerCodepointer");
}

static void LoadMobjMetatable() {
    luaL_newmetatable(L_state, MOBJ_META);

    lua_pushstring(L_state, "__index");
    lua_pushcfunction(L_state, l_mobjIndex);
    lua_rawset(L_state, -3); // set metatable.__index

    lua_pushstring(L_state, "__newindex");
    lua_pushcfunction(L_state, l_mobjNewIndex);
    lua_rawset(L_state, -3); // set metatable.__newindex
}

void CloseLua() {
    lua_close(L_state);
}

static void OpenLua() {
    L_state = luaL_newstate();
    I_AtExit(CloseLua, true);
    luaL_openlibs(L_state);
    LoadLuahackFuncs();
    LoadMobjMetatable();
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
    mobj_t** mobj_lua;

    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    mobj_lua = (mobj_t**) lua_newuserdata(L_state, sizeof(mobj_t*));
    luaL_getmetatable(L_state, MOBJ_META);
    lua_setmetatable(L_state, -2); // set mobj metatable
    *mobj_lua = mobj;
    if (lua_pcall(L_state, 1, 0, 0) != 0) {
        I_Error("%s", lua_tostring(L_state, -1));
    }
}

void CallLuaCptrP2(int cptr) {
    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    if (lua_pcall(L_state, 0, 0, 0) != 0) {
        I_Error("%s", lua_tostring(L_state, -1));
    }
}
