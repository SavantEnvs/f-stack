#!/usr/bin/env bash
#
# mayhem/test.sh — known-answer tests over the components F-Stack bundles and mayhem/ fuzzes.
# build.sh builds one probe per component (Lua + Redis script libraries, Redis ziplist/listpack/intset,
# hiredis, F-Stack's ini parser, nginx's parsers) with the project's normal flags, dynamically linked.
# Each probe feeds fixed inputs through the real code and prints `key=value` results;
# mayhem/kat/expected_<name>.txt holds the values those inputs must produce (JSON round-trip, RESP reply
# types, the epoch of an RFC 1123 date, ...). Every expected line is one test and passes only if the
# probe printed exactly that line. Nothing is compiled here.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
SRC="${SRC:-/mayhem}"
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

passed=0 ; failed=0
for name in lua redis hiredis ini nginx; do
  probe="$SRC/kat_$name"
  expected="$SRC/mayhem/kat/expected_$name.txt"
  if [ ! -x "$probe" ] || [ ! -f "$expected" ]; then
    echo "missing probe or expectations for $name (build.sh should have produced $probe)" >&2
    failed=$((failed + 1)); continue
  fi
  actual="$(timeout 60 "$probe" 2>/dev/null)" || true
  while IFS= read -r line; do
    if printf '%s\n' "$actual" | grep -Fxq -- "$line"; then
      passed=$((passed + 1))
    else
      echo "FAIL $name: expected '$line'" >&2
      failed=$((failed + 1))
    fi
  done <"$expected"
done

emit_ctrf "fstack-known-answer" "$passed" "$failed" 0
