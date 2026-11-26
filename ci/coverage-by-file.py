#!/usr/bin/env python3
# Sum up the coverage maps of a fuzz workspace per compiler source file.
# usage: coverage-by-file.py <basic-block-maps dir> <workspace dir> <output file>
import collections
import glob
import json
import os
import struct
import sys

import numpy as np

MAPS_DIR = sys.argv[1]
WORKSPACE = sys.argv[2]
OUTPUT = sys.argv[3]


def load_coverage(filename):
  with open(filename, 'rb') as fp:
    raw = fp.read()
  assert raw[:4] == b'CVRt'
  size = struct.unpack('4s I 14I', raw[:64])[1]
  bits = np.unpackbits(np.frombuffer(raw[64:], dtype=np.uint8), bitorder='little')
  return bits[:size].astype(bool)


covered = None
for filename in sorted(glob.glob(os.path.join(WORKSPACE, '*', 'coverage.bin'))):
  bits = load_coverage(filename)
  covered = bits if covered is None else (covered | bits)
if covered is None:
  print('no coverage maps found')
  sys.exit(0)

total_by_file = collections.Counter()
covered_by_file = collections.Counter()
for mapfile in glob.glob(os.path.join(MAPS_DIR, '*')):
  with open(mapfile, errors='replace') as fp:
    for line in fp:
      index, _, locations = line.partition(': ')
      try:
        index = int(index)
        source = json.loads(locations)[0][0]
      except (ValueError, IndexError, TypeError):
        continue
      source = source.replace('/root/llvm-src/', '')
      total_by_file[source] += 1
      if index < len(covered) and covered[index]:
        covered_by_file[source] += 1

with open(OUTPUT, 'w') as fp:
  fp.write('covered\ttotal\tfile\n')
  for source, total in sorted(total_by_file.items()):
    fp.write('%d\t%d\t%s\n' % (covered_by_file[source], total, source))
print('%d of %d branches covered in %d files' % (
    sum(covered_by_file.values()), sum(total_by_file.values()), len(total_by_file)))
