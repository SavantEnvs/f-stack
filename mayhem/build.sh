#!/usr/bin/env bash
#
# mayhem/build.sh — F-Stack is a DPDK/FreeBSD userspace network stack that cannot run without hugepages and a
# NIC, so the fuzz targets are the self-contained parsers it bundles: Redis's Lua 5.1 + cjson/cmsgpack/struct,
# Redis's ziplist/listpack/intset validators, hiredis' RESP reader, nginx's HTTP/URL parsers (app/nginx-1.28.0)
# and F-Stack's own ini config parser (lib/ff_ini_parser.c).
#
# Builds, from this one script:
#   1. every dependency of those parsers twice — sanitized (fuzz/standalone) and with normal flags (known-answer probes)
#   2. one libFuzzer binary + one `-standalone` reproducer per harness  -> /mayhem/fuzz_<name>[-standalone]
#   3. the known-answer probes mayhem/test.sh runs                      -> /mayhem/kat_<name>
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

SRC="${SRC:-/mayhem}"
cd "$SRC"

H=mayhem/harnesses
B=mayhem/build
LUA=app/redis-6.2.6/deps/lua/src
REDIS=app/redis-6.2.6/src
HIREDIS=app/redis-6.2.6/deps/hiredis
NGX=app/nginx-1.28.0
NGXOBJS="$SRC/$B/nginx-auto"

rm -rf "$B"
mkdir -p "$B"

# nginx's configure only has to write objs/ngx_auto_config.h and ngx_auto_headers.h; F-Stack's copy then
# fails looking for libdpdk, after those headers exist, which is all the parsers need.
(cd "$NGX" && ./configure --builddir="$NGXOBJS" --without-http_rewrite_module --without-http_gzip_module \
    --without-pcre --without-http_upstream_zone_module) >"$B/nginx-configure.log" 2>&1 || true
[ -f "$NGXOBJS/ngx_auto_config.h" ] && [ -f "$NGXOBJS/ngx_auto_headers.h" ] || { cat "$B/nginx-configure.log"; echo "nginx configure produced no auto headers" >&2; exit 1; }

# nginx's x86 fast path compares method names with whole-word loads that read up to 3 bytes past the token
# (ngx_str6cmp in ngx_http_parse.c). Real nginx always has more request bytes in its 1k+ header buffer, but an
# exact-size fuzz input does not, so ASan flags a read nginx never makes. Setting NGX_HAVE_NONALIGNED to 0 in the
# generated header selects nginx's own byte-wise comparison, which has identical semantics.
sed -i 's/^\(#define NGX_HAVE_NONALIGNED\) .*/\1  0/' "$NGXOBJS/ngx_auto_config.h"

NGX_INC="-D_GNU_SOURCE -I$NGX/src/core -I$NGX/src/event -I$NGX/src/event/modules -I$NGX/src/os/unix -I$NGXOBJS -I$NGX/src/http -I$NGX/src/http/modules"
LUA_SRCS="lapi lauxlib lbaselib lcode ldblib ldebug ldo ldump lfunc lgc linit liolib llex lmathlib lmem loadlib lobject lopcodes loslib lparser lstate lstring lstrlib ltable ltablib ltm lundump lvm lzio lua_bit lua_cjson lua_cmsgpack lua_struct fpconv strbuf"
REDIS_SRCS="zmalloc sds ziplist listpack intset util endianconv sha256 ../../../mayhem/harnesses/redis_stubs"
HIREDIS_SRCS="hiredis read sds alloc net async dict"
NGX_SRCS="src/core/ngx_palloc src/core/ngx_string src/core/ngx_inet src/core/ngx_parse src/core/ngx_parse_time src/core/ngx_array src/core/ngx_list src/core/ngx_hash src/core/ngx_crc32 src/core/ngx_murmurhash src/core/ngx_times src/core/ngx_rbtree src/core/ngx_buf src/os/unix/ngx_alloc src/os/unix/ngx_time src/os/unix/ngx_errno ../../mayhem/harnesses/nginx_stubs src/http/ngx_http_parse"

# build_set <name> <cflags>: compile every dependency into $B/<name>/lib{lua,redis,hiredis,ini,nginx}.a
build_set() {
    local set="$1" flags="$2" out="$B/$1"
    mkdir -p "$out"
    local jobs="$out/jobs.txt"; : >"$jobs"
    local lib f
    for f in $LUA_SRCS;     do echo "lua $LUA/$f.c -DLUA_USE_POSIX -DENABLE_CJSON_GLOBAL -I$LUA" >>"$jobs"; done
    for f in $REDIS_SRCS;   do echo "redis $REDIS/$f.c -I$REDIS -I$HIREDIS" >>"$jobs"; done
    for f in $HIREDIS_SRCS; do echo "hiredis $HIREDIS/$f.c -I$HIREDIS" >>"$jobs"; done
    echo "ini lib/ff_ini_parser.c -Ilib" >>"$jobs"
    for f in $NGX_SRCS;     do echo "nginx $NGX/$f.c $NGX_INC" >>"$jobs"; done
    while read -r lib file extra; do
        mkdir -p "$out/$lib"
        echo "$CC $flags $extra -w -c $file -o $out/$lib/$(basename "$file" .c).o"
    done <"$jobs" | xargs -P "$MAYHEM_JOBS" -I{} sh -c '{}'
    for lib in lua redis hiredis ini nginx; do
        rm -f "$out/lib$lib.a"
        ar rcs "$out/lib$lib.a" "$out/$lib"/*.o
    done
}

# Fuzz/standalone set: ASan+UBSan halting, SanCov on the library code (-fsanitize=fuzzer-no-link), DWARF<=3.
# UBSan checks relaxed because they fire on idioms these C codebases rely on, not on bugs: pointer-overflow
# (Lua/nginx pointer arithmetic), function (nginx's function-pointer casts), nonnull-attribute (memcpy(dst, NULL, 0)),
# float-cast-overflow (Lua 5.1's lua_number2int cast of a double key, luaconf.h) and shift-base
# (lua_cmsgpack.c builds int64 values with `byte << 56`). Everything else, and all of ASan, halts.
FUZZ_FLAGS="$SANITIZER_FLAGS -fno-sanitize=pointer-overflow,function,nonnull-attribute,float-cast-overflow,shift-base -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1"
build_set fuzz "$FUZZ_FLAGS"

# Test set: the project's normal flags (no sanitizers) so mayhem/test.sh stays an honest oracle.
build_set test "-O2 -g0 $COVERAGE_FLAGS"

# $CC $SANITIZER_FLAGS compile of the LSan hook (SPEC: lsan_off.cc linked into every fuzz + standalone binary)
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$B/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"

# harness <name> <includes> <libs...>
harness() {
    local name="$1" inc="$2"; shift 2
    local libs=""; local l
    for l in "$@"; do libs="$libs $B/fuzz/lib$l.a"; done
    $CC $FUZZ_FLAGS -w $inc $LIB_FUZZING_ENGINE "$H/fuzz_$name.c" $libs "$B/lsan_off.o" -lm -ldl -lpthread -o "/mayhem/fuzz_$name"
    $CC $FUZZ_FLAGS -w $inc "$H/fuzz_$name.c" "$B/standalone_main.o" $libs "$B/lsan_off.o" -lm -ldl -lpthread -o "/mayhem/fuzz_$name-standalone"
}

LUA_INC="-I$LUA"
REDIS_INC="-I$REDIS -I$HIREDIS"
for n in lua_parse lua_eval cjson_decode cmsgpack_unpack; do harness "$n" "$LUA_INC -I$H" lua; done
for n in ziplist listpack intset; do harness "$n" "$REDIS_INC" redis; done
harness ini_parse "-Ilib" ini
harness hiredis_reader "-I$HIREDIS" hiredis
for n in nginx_http_request nginx_chunked nginx_url; do harness "$n" "$NGX_INC -I$H" nginx; done

# Known-answer probes, normal flags, dynamically linked (test.sh runs these).
T="-O2 $COVERAGE_FLAGS -w"
K=mayhem/kat
$CC $T -I$LUA -I$H "$K/kat_lua.c" "$B/test/liblua.a" -lm -ldl -o /mayhem/kat_lua
$CC $T $REDIS_INC "$K/kat_redis.c" "$B/test/libredis.a" -lm -o /mayhem/kat_redis
$CC $T -I$HIREDIS "$K/kat_hiredis.c" "$B/test/libhiredis.a" -o /mayhem/kat_hiredis
$CC $T -Ilib "$K/kat_ini.c" "$B/test/libini.a" -o /mayhem/kat_ini
$CC $T $NGX_INC -I$H "$K/kat_nginx.c" "$B/test/libnginx.a" -lm -ldl -lpthread -o /mayhem/kat_nginx
