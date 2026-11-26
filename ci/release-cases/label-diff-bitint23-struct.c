#include <stdio.h>

int main(void) {
  static struct { char before; _BitInt(23) narrow; char after; } record = {7, &&l2 - &&l1, 9};
  static int reference = &&l2 - &&l1;
l1:
  puts("between the labels");
l2:
  printf("bitint=%d int=%d before=%d after=%d\n", (int)record.narrow, reference, record.before, record.after);
  return (int)record.narrow != reference || record.before != 7 || record.after != 9;
}
