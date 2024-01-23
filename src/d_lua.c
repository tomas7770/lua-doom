#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "m_io.h"

#include "doomdef.h"
#include "i_printf.h"
#include "w_wad.h"
#include "memio.h"

#define LUA_BUFFERMAX 1024 // input buffer area size, hardcoded for now

void ProcessLuaLump(int lumpnum)
{
    MEMFILE* lump;
    char inbuffer[LUA_BUFFERMAX];
    int error;
    lua_State *L;

    {
        void *buf = W_CacheLumpNum(lumpnum, PU_STATIC);

        lump = mem_fopen_read(buf, W_LumpLength(lumpnum));
    }

    L = luaL_newstate();
    luaL_openlibs(L);

    // loop until end of file
    while (mem_fgets(inbuffer, sizeof(inbuffer), lump))
    {
        error = luaL_loadbuffer(L, inbuffer, strlen(inbuffer), "line") ||
                lua_pcall(L, 0, 0, 0);
        if (error) {
            fprintf(stderr, "%s", lua_tostring(L, -1));
            lua_pop(L, 1);  /* pop error message from the stack */
        }
    }

    lua_close(L);

    mem_fclose(lump);
}
