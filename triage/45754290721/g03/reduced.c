typedef int aligned __attribute__((aligned(64)));
int bar();
void foo() {
  char *p = __builtin_alloca(-77);
  aligned i;
  bar(p) || bar(&i);
}
