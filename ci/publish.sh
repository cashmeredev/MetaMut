#!/bin/bash
# Commit a directory of result files to the ci-results branch.
# usage: publish.sh <checkout dir> <source dir> <target path in branch> <commit message>
set -e

CHECKOUT=$1
SOURCE=$(realpath "$2")
TARGET=$3
MESSAGE=$4

cd "$CHECKOUT"
git config user.name "github-actions[bot]"
git config user.email "41898282+github-actions[bot]@users.noreply.github.com"
for attempt in 1 2 3 4 5 6; do
  if git fetch -q --depth 1 origin ci-results; then
    git checkout -q -B ci-results FETCH_HEAD
  else
    git checkout -q --orphan ci-results
    git rm -rqf . || true
  fi
  mkdir -p "$TARGET"
  cp -r "$SOURCE/." "$TARGET/"
  git add -A "$TARGET"
  git commit -q -m "$MESSAGE" || exit 0
  if git push -q origin ci-results; then
    exit 0
  fi
  sleep $((attempt * 7))
done
exit 1
