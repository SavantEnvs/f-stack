/* Lua 5.1 state with the library set Redis exposes to EVAL scripts (app/redis-6.2.6/src/scripting.c
 * luaLoadLibraries): no io/os/package. The debug library is left out: it can rewrite upvalues, locals and
 * the instruction hook, which crashes any Lua 5.1 state by design and hides the bugs in the other libraries. */
#ifndef MAYHEM_LUA_ENV_H
#define MAYHEM_LUA_ENV_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

LUALIB_API int (luaopen_cjson)(lua_State *L);
LUALIB_API int (luaopen_struct)(lua_State *L);
LUALIB_API int (luaopen_cmsgpack)(lua_State *L);
LUALIB_API int (luaopen_bit)(lua_State *L);

#include <stdlib.h>

/* Deterministic bound on a script's memory (no timers): allocations past the cap fail like an out-of-memory
 * Lua state, which Lua reports as an ordinary error. */
#define MAYHEM_LUA_MEM_CAP (128u << 20)
static void *mayhem_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    size_t *used = ud;
    if (nsize == 0) { *used -= ptr ? osize : 0; free(ptr); return NULL; }
    if (*used - (ptr ? osize : 0) + nsize > MAYHEM_LUA_MEM_CAP) return NULL;
    void *n = realloc(ptr, nsize);
    if (n) *used += nsize - (ptr ? osize : 0);
    return n;
}

static void mayhem_load_lib(lua_State *L, const char *name, lua_CFunction f)
{
    lua_pushcfunction(L, f);
    lua_pushstring(L, name);
    lua_call(L, 1, 0);
}

static size_t mayhem_mem_used;

static lua_State *mayhem_new_state(void)
{
    mayhem_mem_used = 0;
    lua_State *L = lua_newstate(mayhem_alloc, &mayhem_mem_used);
    if (!L) return NULL;
    mayhem_load_lib(L, "", luaopen_base);
    mayhem_load_lib(L, LUA_TABLIBNAME, luaopen_table);
    mayhem_load_lib(L, LUA_STRLIBNAME, luaopen_string);
    mayhem_load_lib(L, LUA_MATHLIBNAME, luaopen_math);
    mayhem_load_lib(L, "cjson", luaopen_cjson);
    mayhem_load_lib(L, "struct", luaopen_struct);
    mayhem_load_lib(L, "cmsgpack", luaopen_cmsgpack);
    mayhem_load_lib(L, "bit", luaopen_bit);
    /* Redis removes these from the script environment; print is dropped too
     * so a script cannot flood stdout. */
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "print");
    return L;
}

#endif
