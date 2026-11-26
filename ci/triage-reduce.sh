#!/bin/bash
# Shrink the program of one crash group with cvise while the compiler keeps
# failing in the same way.
# usage: triage-reduce.sh <clang> <group dir> <compiler options> <minutes>
set -u

CLANG=$(realpath "$1")
GROUP=$(realpath "$2")
OPTIONS=$3
MINUTES=$4

cd "$GROUP"
cp original.c reduced.c
valid=$(python3 -c "import json; print(json.load(open('info.json'))['valid'])")

# cvise calls this script in a scratch directory that holds reduced.c
cat > interesting.sh <<SCRIPT
#!/bin/bash
timeout 60 "$CLANG" reduced.c -o /dev/null -w -fno-crash-diagnostics $OPTIONS > out.txt 2>&1
grep -qFf "$GROUP/signature.txt" out.txt || exit 1
SCRIPT
if [ "$valid" = "True" ]; then
  # the original compiles without errors, the reduced one has to as well
  echo "! grep -qE ':[0-9]+:[0-9]+: error:' out.txt" >> interesting.sh
fi
chmod +x interesting.sh

if ! ./interesting.sh; then
  echo "the crash does not reproduce before reduction" > reduce-log.txt
  rm -f out.txt
  exit 0
fi

timeout "${MINUTES}m" cvise --n "$(nproc)" --timeout 90 ./interesting.sh reduced.c > reduce-log-full.txt 2>&1
echo "cvise exit=$?" >> reduce-log-full.txt
tail -c 3000 reduce-log-full.txt > reduce-log.txt
rm -f reduce-log-full.txt reduced.c.orig

if ./interesting.sh; then
  cp out.txt reduced-stderr.txt
  echo "reduced: $(wc -c < original.c) -> $(wc -c < reduced.c) bytes, $(wc -l < reduced.c) lines" >> reduce-log.txt
else
  echo "the reduced file no longer reproduces, keeping the original" >> reduce-log.txt
  cp original.c reduced.c
fi
rm -f out.txt
exit 0
