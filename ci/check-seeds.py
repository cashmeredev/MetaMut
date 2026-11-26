#!/usr/bin/env python3
# Compile every seed with the compiler under test and list the seeds that
# crash it on their own, so the fuzzer does not report them again and again.
# usage: check-seeds.py <clang> <seeds dir> <output file> <compiler options>
import os
import shlex
import subprocess
import sys
from multiprocessing import Pool

CLANG = sys.argv[1]
SEEDS_DIR = sys.argv[2]
OUTPUT = sys.argv[3]
OPTIONS = shlex.split(sys.argv[4])


def check(name):
  path = os.path.join(SEEDS_DIR, name)
  cmd = [CLANG, path, '-o', '/dev/null', '-w', '-fno-crash-diagnostics'] + OPTIONS
  try:
    proc = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
        stdin=subprocess.DEVNULL, timeout=20)
  except subprocess.TimeoutExpired:
    return name, 'timeout'
  stderr = proc.stderr.decode('utf-8', errors='replace')
  if 'PLEASE submit a bug report' not in stderr and proc.returncode in (0, 1):
    return name, None
  reason = 'exit code %d' % proc.returncode
  for line in stderr.splitlines():
    if 'Assertion' in line or 'UNREACHABLE' in line or 'LLVM ERROR' in line:
      reason = line.strip()[:300]
      break
  return name, reason


if __name__ == '__main__':
  names = sorted(os.listdir(SEEDS_DIR))
  crashing = []
  with Pool(os.cpu_count()) as pool:
    for name, reason in pool.imap_unordered(check, names, chunksize=50):
      if reason is not None:
        crashing.append((name, reason))
  crashing.sort()
  with open(OUTPUT, 'w') as fp:
    for name, reason in crashing:
      fp.write('%s\t%s\n' % (name, reason))
  print('%d of %d seeds crash or hang the compiler' % (len(crashing), len(names)))
