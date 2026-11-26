typedef int v4si __attribute__((vector_size(16)));
_BitInt(67) s;
v4si foo(v4si a) { return a + s; }
