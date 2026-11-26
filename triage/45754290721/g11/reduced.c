struct {
  char x[804289383]
} foo_x, foo_y, foo_z;
void foo() { __atomic_exchange(&foo_x, &foo_y, &foo_z, __ATOMIC_SEQ_CST); }
