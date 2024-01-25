#ifndef __D_LUA__
#define __D_LUA__

#include "p_mobj.h"

#define LUA_CPTR_NAME_SIZE 128

typedef struct {
    int cptr;  // actual pointer to the subroutine
    char lookup[LUA_CPTR_NAME_SIZE];  // mnemonic lookup string to be specified in BEX
} lua_cptr;

void ProcessLuaLump(int lumpnum);
void CallLuaCptrP1(int cptr, mobj_t* mobj);
void CallLuaCptrP2(int cptr);

#endif
