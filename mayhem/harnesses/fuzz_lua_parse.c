/* Lua 5.1 compiler (llex/lparser/lcode) as shipped in Redis' deps/lua. Compile only, never run. */
#include <stddef.h>
#include <stdint.h>
#include "lua_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Lua 5.1's lundump.c trusts precompiled chunks (no bytecode verifier), so a chunk starting
     * with LUA_SIGNATURE[0] is outside the loader's contract for untrusted input. */
    if (size > 0 && data[0] == (uint8_t)LUA_SIGNATURE[0]) return -1;

    lua_State *L = luaL_newstate();
    if (!L) return -1;
    int rc = luaL_loadbuffer(L, (const char *)data, size, "=fuzz");
    lua_close(L);
    return rc ? -1 : 0;
}
