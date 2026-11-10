/* Run an arbitrary script in the Lua 5.1 sandbox Redis gives EVAL scripts
 * (base, table, string, math, cjson, struct, cmsgpack, bit). */
#include <stddef.h>
#include <stdint.h>
#include "lua_env.h"

/* Instruction budget instead of a timer: a script that loops forever stops with an ordinary Lua error. */
static void budget_hook(lua_State *L, lua_Debug *ar)
{
    (void)ar;
    luaL_error(L, "instruction budget exceeded");
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* See fuzz_lua_parse.c: precompiled chunks are not verified by Lua 5.1. */
    if (size > 0 && data[0] == (uint8_t)LUA_SIGNATURE[0]) return -1;

    lua_State *L = mayhem_new_state();
    if (!L) return -1;
    if (luaL_loadbuffer(L, (const char *)data, size, "=fuzz")) {
        lua_close(L);
        return -1;
    }
    lua_sethook(L, budget_hook, LUA_MASKCOUNT, 1000000);
    lua_pcall(L, 0, 0, 0);
    lua_close(L);
    return 0;
}
