#ifndef __D_LUA__
#define __D_LUA__

#define LUA_CPTR_NAME_SIZE 128

typedef struct {
    int cptr;  // actual pointer to the subroutine
    char lookup[LUA_CPTR_NAME_SIZE];  // mnemonic lookup string to be specified in BEX
} lua_cptr;

void ProcessLuaLump(int lumpnum);
void CallLuaCptr(int cptr);

#endif
