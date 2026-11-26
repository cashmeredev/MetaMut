#!/usr/bin/env python3
# Re-run every crashing program of one fuzz run, record how the compiler
# fails and group the programs by that failure.
# usage: triage-classify.py <clang> <results dir of the fuzz run> <output dir> <compiler options>
import collections
import glob
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from multiprocessing import Pool

CLANG = sys.argv[1]
RESULTS = sys.argv[2]
OUTPUT = sys.argv[3]
OPTIONS = shlex.split(sys.argv[4])
LEVELS = ['-O0', '-O1', '-O2', '-O3']

FRAME_PATTERN = re.compile(r'^\s*#\d+ 0x[0-9a-f]+ (.+?)(?: \(/|$)')
DIAG_ERROR_PATTERN = re.compile(r':\d+:\d+: error:')
MESSAGE_PATTERNS = [
  re.compile(r"Assertion `.*' failed\."),
  re.compile(r'UNREACHABLE executed at \S+'),
  re.compile(r'fatal error: error in backend: .*'),
  re.compile(r'LLVM ERROR: .*'),
]
# frames of the crash handling itself, they say nothing about the cause
NOISE_FRAMES = ['PrintStackTrace', 'RunSignalHandlers', 'SignalHandler',
  'CleanupOnSignal', 'CrashRecovery', 'report_fatal_error',
  'reportFatal', 'llvm_unreachable_internal', '__assert', 'abort', 'raise',
  'pthread_kill', '__restore_rt', 'libc.so', '__GI_']


def run_clang(path, options):
  cmd = [CLANG, path, '-o', '/dev/null', '-w', '-fno-crash-diagnostics'] + options
  try:
    proc = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
        stdin=subprocess.DEVNULL, timeout=120)
  except subprocess.TimeoutExpired:
    return None, 'timeout'
  return proc.returncode, proc.stderr.decode('utf-8', errors='replace')


def is_crash(retcode, stderr):
  if retcode is None:
    return False
  return 'PLEASE submit a bug report' in stderr or retcode not in (0, 1)


def get_frames(stderr):
  frames = []
  for line in stderr.splitlines():
    match = FRAME_PATTERN.match(line)
    if not match:
      continue
    frame = match.group(1).strip()
    if any(noise in frame for noise in NOISE_FRAMES):
      continue
    # drop the argument list, it makes equal frames look different
    frame = frame.split('(')[0].strip()[:120]
    if frame:
      frames.append(frame)
  return frames


def get_signature(stderr):
  for pattern in MESSAGE_PATTERNS:
    match = pattern.search(stderr)
    if match:
      return 'message', match.group(0)[:400]
  frames = get_frames(stderr)
  if frames:
    return 'frame', frames[0]
  return 'unknown', 'no message and no stack trace'


def classify(item):
  path, origin = item
  retcode, stderr = run_clang(path, OPTIONS)
  result = {'path': path, 'origin': origin, 'size': os.path.getsize(path),
      'crashes': is_crash(retcode, stderr), 'timeout': retcode is None}
  if not result['crashes']:
    return result
  result['kind'], result['signature'] = get_signature(stderr)
  result['frames'] = get_frames(stderr)[:8]
  result['stderr'] = stderr
  result['valid'] = not DIAG_ERROR_PATTERN.search(stderr)
  # which optimization levels crash in the same way
  base_options = [opt for opt in OPTIONS if opt not in LEVELS]
  result['levels'] = []
  for level in LEVELS:
    code, text = run_clang(path, base_options + [level])
    if is_crash(code, text) and get_signature(text)[1] == result['signature']:
      result['levels'].append(level)
  return result


def find_inputs():
  items = []
  for instance in sorted(glob.glob(os.path.join(RESULTS, '*', '*,*,*'))):
    custom = set()
    log = os.path.join(instance, 'crash-log.txt')
    if os.path.exists(log):
      for line in open(log):
        if not line.strip():
          continue
        record = json.loads(line)
        if 'output/mu.c ' in record['invoke']:
          custom.add(os.path.basename(record['crash'][1]))
    for path in sorted(glob.glob(os.path.join(instance, 'crash-inputs', '*.c'))):
      origin = {'job': os.path.basename(os.path.dirname(instance)),
          'instance': os.path.basename(instance),
          'custom': os.path.basename(path) in custom}
      items.append((path, origin))
  return items


if __name__ == '__main__':
  items = find_inputs()
  with Pool(os.cpu_count()) as pool:
    results = pool.map(classify, items)

  groups = collections.defaultdict(list)
  for result in results:
    if result['crashes']:
      groups[result['signature']].append(result)

  os.makedirs(OUTPUT, exist_ok=True)
  ordered = sorted(groups.items(), key=lambda kv: (-len(kv[1]), kv[0]))
  summary = []
  for number, (signature, members) in enumerate(ordered, start=1):
    group_id = 'g%02d' % number
    members.sort(key=lambda m: m['size'])
    smallest = members[0]
    group_dir = os.path.join(OUTPUT, group_id)
    os.makedirs(group_dir, exist_ok=True)
    shutil.copy(smallest['path'], os.path.join(group_dir, 'original.c'))
    with open(os.path.join(group_dir, 'stderr.txt'), 'w') as fp:
      fp.write(smallest['stderr'])
    with open(os.path.join(group_dir, 'signature.txt'), 'w') as fp:
      fp.write(signature + '\n')
    info = {
      'id': group_id,
      'kind': smallest['kind'],
      'signature': signature,
      'frames': smallest['frames'],
      'inputs': len(members),
      'jobs': sorted(set(m['origin']['job'] for m in members)),
      'by_custom_mutation': sum(1 for m in members if m['origin']['custom']),
      'levels': smallest['levels'],
      'valid': all(m['valid'] for m in members),
      'smallest': os.path.basename(smallest['path']),
      'smallest_size': smallest['size'],
      'files': [m['origin']['instance'] + '/' + os.path.basename(m['path']) for m in members],
    }
    with open(os.path.join(group_dir, 'info.json'), 'w') as fp:
      json.dump(info, fp, indent=1)
    summary.append(info)

  not_reproduced = [r['origin']['instance'] + '/' + os.path.basename(r['path'])
      for r in results if not r['crashes']]
  with open(os.path.join(OUTPUT, 'groups.json'), 'w') as fp:
    json.dump({'groups': summary, 'not_reproduced': not_reproduced}, fp, indent=1)
  with open(os.path.join(OUTPUT, 'groups.txt'), 'w') as fp:
    for info in summary:
      fp.write('%s  inputs=%d  custom=%d  levels=%s  valid=%s  jobs=%s\n' % (
          info['id'], info['inputs'], info['by_custom_mutation'],
          ','.join(info['levels']), info['valid'], ','.join(info['jobs'])))
      fp.write('    %s\n' % info['signature'])
      for frame in info['frames'][:3]:
        fp.write('      at %s\n' % frame)
    fp.write('not reproduced: %d\n' % len(not_reproduced))
    for name in not_reproduced:
      fp.write('    %s\n' % name)
  with open(os.path.join(OUTPUT, 'matrix.json'), 'w') as fp:
    json.dump([info['id'] for info in summary], fp)
  print('%d inputs, %d crash, %d groups' % (
      len(results), sum(1 for r in results if r['crashes']), len(summary)))
