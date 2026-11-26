// options: -std=c2x -O2 -c
typedef int v4si __attribute__((vector_size(16)));
v4si foo(v4si a) { return a + 66666666666666666666wb; }
