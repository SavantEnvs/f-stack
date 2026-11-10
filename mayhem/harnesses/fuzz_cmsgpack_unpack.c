/* lua-cmsgpack (Redis' deps/lua/src/lua_cmsgpack.c): cmsgpack.unpack on raw bytes, then re-pack the values. */
#include <stddef.h>
#include <stdint.h>
#include "lua_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    lua_State *L = mayhem_new_state();
    if (!L) return -1;

    lua_getglobal(L, "cmsgpack");
    lua_getfield(L, -1, "unpack");
    lua_pushlstring(L, (const char *)data, size);
    int base = lua_gettop(L) - 2;
    if (lua_pcall(L, 1, LUA_MULTRET, 0)) {
        lua_close(L);
        return -1;
    }
    int n = lua_gettop(L) - base;
    if (n > 0 && lua_checkstack(L, n + 2)) {
        lua_getglobal(L, "cmsgpack");
        lua_getfield(L, -1, "pack");
        for (int i = 1; i <= n; i++) lua_pushvalue(L, base + i);
        lua_pcall(L, n, 1, 0);
    }
    lua_close(L);
    return 0;
}
