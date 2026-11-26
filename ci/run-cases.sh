#!/bin/bash
# Compile every hand-written case and record how the compiler reacts.
# usage: run-cases.sh <clang> <cases dir> <out dir>
# A case can set its own options in the first line: // options: -O0 -c
set -u

CLANG=$(realpath "$1")
CASES=$(realpath "$2")
OUT=$3

mkdir -p "$OUT"
OUT=$(realpath "$OUT")
: > "$OUT/results.txt"

for file in "$CASES"/*.c; do
  name=$(basename "$file" .c)
  options=$(sed -n '1s|^// options: ||p' "$file")
  if [ -z "$options" ]; then
    options="-O2 -c"
  fi
  # shellcheck disable=SC2086
  timeout 120 "$CLANG" $options -w -o /dev/null "$file" > /dev/null 2> "$OUT/$name.stderr.txt"
  status=$?
  if grep -q -e 'Assertion' -e 'UNREACHABLE' -e 'Stack dump:' -e 'LLVM ERROR' "$OUT/$name.stderr.txt"; then
    result=crash
    detail=$(grep -m1 -o -e 'Assertion .*' -e 'UNREACHABLE.*' -e 'LLVM ERROR.*' "$OUT/$name.stderr.txt")
  elif [ "$status" -eq 124 ]; then
    result=timeout
    detail=""
  elif [ "$status" -eq 0 ]; then
    result=compiles
    detail=""
  else
    result=rejected
    detail=$(grep -m1 'error:' "$OUT/$name.stderr.txt" | sed 's|^.*error: ||')
  fi
  printf '%s\t%s\t%s\t%s\n' "$name" "$result" "$options" "$detail" >> "$OUT/results.txt"
done

cat "$OUT/results.txt"
