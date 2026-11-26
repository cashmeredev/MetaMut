#!/bin/bash
timeout 60 "/home/runner/work/MetaMut/MetaMut/clang/bin/clang-24" reduced.c -o /dev/null -w -fno-crash-diagnostics -O2 -c > out.txt 2>&1
grep -qFf "/home/runner/work/MetaMut/MetaMut/triage/g07/signature.txt" out.txt || exit 1
! grep -qE ':[0-9]+:[0-9]+: error:' out.txt
