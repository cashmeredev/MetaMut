#!/bin/bash
# Build clang from one llvm-project commit inside the metamut docker image.
# usage: build-clang.sh <plain|instrum> <llvm commit>
# Expects /root/build and /out to be mounted from the host.
set -e

MODE=$1
LLVM_SHA=$2
JOBS=$(nproc)
INSTRUM_PATH=/root/MetaMut/instrumenter/output/afl-bins

# the image ships a cmake that is too old for current llvm and lacks the
# headers the gcc instrumentation plugin is built against
apt-get update -qq
apt-get install -y -qq gcc-11-plugin-dev python3-pip > /dev/null
pip3 install -q 'cmake>=3.31'
hash -r
cmake --version | head -n1

mkdir -p /root/llvm-src && cd /root/llvm-src
git init -q
git remote add origin https://github.com/llvm/llvm-project
git fetch -q --depth 1 origin "$LLVM_SHA"
git checkout -q FETCH_HEAD
git log -1 --format='%H %cd %s' > /out/llvm-commit.txt

CMAKE_ARGS="-DLLVM_ENABLE_PROJECTS=clang -DLLVM_TARGETS_TO_BUILD=X86"
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON"
CMAKE_ARGS="$CMAKE_ARGS -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF"
# linking the instrumented clang needs more memory than the runner has
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_EXE_LINKER_FLAGS=-Wl,--no-keep-memory"

cd /root/build
if [ "$MODE" = "plain" ]; then
  cmake /root/llvm-src/llvm $CMAKE_ARGS
  make clang -j "$JOBS"
else
  if ! [ -x "$INSTRUM_PATH/afl-gcc-fast" ]; then
    echo "[*] building the instrumenter"
    mkdir -p /root/MetaMut/instrumenter/output
    cd /root/MetaMut/instrumenter/output
    cmake .. -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_CXX_COMPILER=g++-11
    make -j "$JOBS"
    cd /root/build
  fi
  cmake /root/llvm-src/llvm $CMAKE_ARGS \
    -DCMAKE_C_COMPILER="$INSTRUM_PATH/afl-gcc-fast" \
    -DCMAKE_CXX_COMPILER="$INSTRUM_PATH/afl-g++-fast"
  python3 "$INSTRUM_PATH/server.py" -o configs.json -- make clang -j "$JOBS"
  cp configs.json /out/configs.json
fi

/root/build/bin/clang --version > /out/clang-version.txt
echo 'int main(void) { return 0; }' > /tmp/hello.c
/root/build/bin/clang -O2 -c /tmp/hello.c -o /tmp/hello.o
echo "build ok" >> /out/clang-version.txt
