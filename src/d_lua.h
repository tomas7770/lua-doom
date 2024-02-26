#ifndef __D_LUA__
#define __D_LUA__

#include <lua.h>

#include "p_mobj.h"
#include "d_player.h"

#define LUA_CPTR_NAME_SIZE 128

typedef struct {
    int cptr;  // actual pointer to the subroutine
    char lookup[LUA_CPTR_NAME_SIZE];  // mnemonic lookup string to be specified in BEX
} lua_cptr;

void ProcessLuaLump(int lumpnum);
void CallLuaCptrP1(int cptr, mobj_t* mobj, long args[]);
void CallLuaCptrP2(int cptr, player_t* player, pspdef_t* psp, long args[]);
fixed_t ToFixed(double x);
double FromFixed(fixed_t x);
void** NewUserdata(lua_State* L, void* data, size_t data_size, const char* metatable);

#endif
