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

extern actionf_t FindDehCodepointer(const char* key);
extern char *ptr_lstrip(char *p);

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

static int l_tofixed(lua_State* L) {
    double x = luaL_checknumber(L, 1);
    int result = FRACUNIT*x;
    lua_pushinteger(L, result);
    return 1;
}

static mobj_t** NewMobj(lua_State* L, mobj_t* mobj) {
    mobj_t** mobj_lua = (mobj_t**) lua_newuserdata(L, sizeof(mobj_t*));
    luaL_getmetatable(L, MOBJ_META);
    lua_setmetatable(L, -2); // set mobj metatable
    *mobj_lua = mobj;
    return mobj_lua;
}

static mobj_t** CheckMobj(lua_State* L) {
    void *ud = luaL_checkudata(L, 1, MOBJ_META);
    luaL_argcheck(L, ud != NULL, 1, "'mobj' expected");
    return (mobj_t**) ud;
}

// Is this necessary? Why not jump to another state with the intended codepointer?
static int l_mobj_call(lua_State* L) {
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
        int extra_args = lua_gettop(L)-2;
        memcpy(orig_args, (*mobj_lua)->state->args, sizeof(orig_args));
        for (j = 0; j < extra_args && j < MAXSTATEARGS; j++) {
            int value;
            if (lua_isnil(L, j+1+2)) {
                // Skip arg
                continue;
            }
            value = luaL_checkinteger(L, j+1+2);
            // Changes the state globally for all mobjs of this type, quite a hack...
            // But atleast it's restored later
            (*mobj_lua)->state->args[j] = value;
        }

        c_cptr.p1(*mobj_lua);
        memcpy((*mobj_lua)->state->args, orig_args, sizeof(orig_args)); // restore state args
        found = true;
    }
    else {
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

// call(...) but for Misc1/Misc2 arguments (only valid for built-in codepointers)
static int l_mobj_callMisc(lua_State* L) {
    char key[LUA_CPTR_NAME_SIZE];
    actionf_t c_cptr;
    mobj_t** mobj_lua = CheckMobj(L);
    const char* cptr_name = luaL_checkstring(L, 2);
    boolean found = false;
    
    strcpy(key,"A_");  // reusing the key area to prefix the mnemonic
    strcat(key, ptr_lstrip((char*) cptr_name));
    
    c_cptr = FindDehCodepointer(key);
    if (c_cptr.p1) {
        // Count args and apply them
        int j;
        long orig_misc1, orig_misc2;
        int extra_args = lua_gettop(L)-2;
        orig_misc1 = (*mobj_lua)->state->misc1;
        orig_misc2 = (*mobj_lua)->state->misc2;
        for (j = 0; j < extra_args && j < 2; j++) {
            int value;
            if (lua_isnil(L, j+1+2)) {
                // Skip arg
                continue;
            }
            value = luaL_checkinteger(L, j+1+2);
            // Changes the state globally for all mobjs of this type, quite a hack...
            // But atleast it's restored later
            switch (j) {
                case 0:
                    (*mobj_lua)->state->misc1 = value;
                    break;
                case 1:
                    (*mobj_lua)->state->misc2 = value;
                    break;
                default:
                    // This isn't going to happen...
                    break;
            }
        }

        c_cptr.p1(*mobj_lua);
        (*mobj_lua)->state->misc1 = orig_misc1; // restore state args
        (*mobj_lua)->state->misc2 = orig_misc2;
        found = true;
    }

    if (!found) {
        luaL_argerror(L, 2, "codepointer not found");
    }

    return 0;
}

static int l_mobjIndex(lua_State* L) {
    mobj_t** mobj_lua = CheckMobj(L);

    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "health") == 0) {
        lua_pushnumber(L, (*mobj_lua)->health);
    }
    else if (strcmp(key, "target") == 0) {
        if ((*mobj_lua)->target) {
            NewMobj(L, (*mobj_lua)->target);
            lua_pushvalue(L, -1);
        }
        else {
            lua_pushnil(L);
        }
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
    else {
        luaL_argerror(L, 2, "invalid mobj attribute");
    }

    return 0;
}

static void LoadLuahackFuncs() {
    lua_pushcfunction(L_state, l_registerCodepointer);
    lua_setglobal(L_state, "registerCodepointer");
    lua_pushcfunction(L_state, l_tofixed);
    lua_setglobal(L_state, "tofixed");
}

static const struct luaL_Reg mobj_lib[] = {
    {"call", l_mobj_call},
    {"callMisc", l_mobj_callMisc},
    {NULL, NULL}
};

static void LoadMobjMetatable() {
    luaL_newmetatable(L_state, MOBJ_META);

    luaL_setfuncs(L_state, mobj_lib, 0);

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
    lua_rawgeti(L_state, LUA_REGISTRYINDEX, cptr);
    NewMobj(L_state, mobj);
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
