/* lua-cjson (Redis' deps/lua/src/lua_cjson.c): cjson.decode on raw bytes, then re-encode the result. */
#include <stddef.h>
#include <stdint.h>
#include "lua_env.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    lua_State *L = mayhem_new_state();
    if (!L) return -1;

    lua_getglobal(L, "cjson");
    lua_getfield(L, -1, "decode");
    lua_pushlstring(L, (const char *)data, size);
    if (lua_pcall(L, 1, 1, 0)) {
        lua_close(L);
        return -1;
    }
    lua_getglobal(L, "cjson");
    lua_getfield(L, -1, "encode");
    lua_pushvalue(L, -3);
    lua_pcall(L, 1, 1, 0);
    lua_close(L);
    return 0;
}
