#!/bin/bash
# Copy the small result files of a fuzz workspace into a report directory.
# usage: collect.sh <workspace dir> <report dir>
set -e

WORKSPACE=$1
REPORT=$2

mkdir -p "$REPORT"
[ -f "$WORKSPACE/run.log" ] && tail -c 20000 "$WORKSPACE/run.log" > "$REPORT/run-tail.log"
[ -f "$WORKSPACE/summary.txt" ] && cp "$WORKSPACE/summary.txt" "$REPORT/"

for dir in "$WORKSPACE"/*,*,*; do
  [ -d "$dir" ] || continue
  name=$(basename "$dir")
  mkdir -p "$REPORT/$name"
  for file in coverage-trend.txt crashes.json state.txt exceptions.txt; do
    [ -f "$dir/$file" ] && cp "$dir/$file" "$REPORT/$name/"
  done
  [ -f "$dir/coverage.bin" ] && zstd -q -19 "$dir/coverage.bin" -o "$REPORT/$name/coverage.bin.zst"
  if [ -f "$dir/log.txt" ]; then
    tail -c 20000 "$dir/log.txt" > "$REPORT/$name/log-tail.txt"
    grep '"crash"' "$dir/log.txt" > "$REPORT/$name/crash-log.txt" || true
    {
      echo "kept=$(grep -c '"keep"' "$dir/log.txt" || true)"
      echo "kept_by_custom=$(grep '"keep"' "$dir/log.txt" | grep -c 'output/mu\.c ' || true)"
      echo "crashes=$(grep -c '"crash"' "$dir/log.txt" || true)"
      echo "crashes_by_custom=$(grep '"crash"' "$dir/log.txt" | grep -c 'output/mu\.c ' || true)"
      echo "crashes_first_hit=$(grep '"crash"' "$dir/log.txt" | grep -c '"inherited": false' || true)"
    } > "$REPORT/$name/counts.txt"
  fi
  # keep only the source files that crashes.json refers to
  if [ -f "$dir/crashes.json" ]; then
    mkdir -p "$REPORT/$name/crash-inputs"
    grep -oE '[A-Za-z]{12}\.c' "$dir/crashes.json" | sort -u | while read -r source; do
      [ -f "$dir/$source" ] && cp "$dir/$source" "$REPORT/$name/crash-inputs/"
    done
  fi
done
exit 0
