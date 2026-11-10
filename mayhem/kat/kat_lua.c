/* Known-answer probe: run fixed scripts through the Lua 5.1 + Redis script libraries and print each result. */
#include <stdio.h>
#include <string.h>
#include "../harnesses/lua_env.h"

static const char *scripts[][2] = {
    {"arith",       "return 6*7"},
    {"format",      "return string.format('%d-%s', 5, 'x')"},
    {"cjson_enc",   "return cjson.encode({1,2,3})"},
    {"cjson_dec",   "return cjson.decode('{\"a\":[1,2,{\"b\":7}]}').a[3].b"},
    {"struct_pack", "local s = struct.pack('>I2', 258); return s:byte(1)*1000 + s:byte(2)"},
    {"bit_band",    "return bit.band(0xff, 0x0f)"},
    {"mp_len",      "return #cmsgpack.pack({1,2,3})"},
    {"mp_round",    "return cmsgpack.unpack(cmsgpack.pack('hi'))"},
    {"sort",        "local t = {5,3,9,1}; table.sort(t); return table.concat(t, ',')"},
    {"gsub",        "return (string.gsub('hello world', 'o', '0'))"},
};

int main(void)
{
    for (size_t i = 0; i < sizeof(scripts) / sizeof(scripts[0]); i++) {
        lua_State *L = mayhem_new_state();
        const char *res = "error";
        if (L && luaL_loadstring(L, scripts[i][1]) == 0 && lua_pcall(L, 0, 1, 0) == 0)
            res = lua_tostring(L, -1);
        printf("lua.%s=%s\n", scripts[i][0], res ? res : "nil");
        if (L) lua_close(L);
    }
    return 0;
}
