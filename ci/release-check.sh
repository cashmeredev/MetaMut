#!/bin/bash
# Build and run the self-checking programs with a clang that has no
# assertions and record whether the result is right.
# usage: release-check.sh <clang> <label> <cases dir> <control case> <out dir>
# The control case is a program that only fails an assertion; if it compiles,
# this clang really is built without assertions.
set -u

CLANG=$1
LABEL=$2
CASES=$(realpath "$3")
CONTROL=$(realpath "$4")
OUT=$5

mkdir -p "$OUT/$LABEL"
OUT=$(realpath "$OUT")
WORK=$(mktemp -d)

"$CLANG" --version | head -n1 > "$OUT/$LABEL/version.txt"

if "$CLANG" -O2 -c -w -o /dev/null "$CONTROL" 2> "$OUT/$LABEL/control.stderr.txt"; then
  control=compiles
else
  control=fails
fi
printf '%s\t%s\t%s\t%s\t%s\n' "$LABEL" control - "$control" "" >> "$OUT/results.txt"

for file in "$CASES"/*.c; do
  name=$(basename "$file" .c)
  for level in -O0 -O2; do
    log="$OUT/$LABEL/$name$level"
    if ! timeout 120 "$CLANG" $level -w -o "$WORK/program" "$file" 2> "$log.stderr.txt"; then
      if grep -q -e 'Assertion' -e 'Stack dump:' "$log.stderr.txt"; then
        result=compiler-crash
      else
        result=rejected
      fi
      printf '%s\t%s\t%s\t%s\t%s\n' "$LABEL" "$name" "$level" "$result" "" >> "$OUT/results.txt"
      continue
    fi
    output=$(timeout 10 "$WORK/program" 2>&1 | tail -n1)
    status=${PIPESTATUS[0]}
    if [ "$status" -eq 0 ]; then
      result=right
    else
      result="wrong(exit $status)"
    fi
    printf '%s\t%s\t%s\t%s\t%s\n' "$LABEL" "$name" "$level" "$result" "$output" >> "$OUT/results.txt"
    "$CLANG" $level -w -S -emit-llvm -o - "$file" 2> /dev/null | grep -e '^@' > "$log.globals.ll"
  done
done
rm -rf "$WORK"
