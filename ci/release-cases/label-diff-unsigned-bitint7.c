#include <stdio.h>

int main(void) {
  static unsigned _BitInt(7) narrow = &&l2 - &&l1;
  static int reference = &&l2 - &&l1;
l1:
  puts("between the labels");
l2:
  printf("bitint=%d int=%d\n", (int)narrow, reference & 0x7f);
  return (int)narrow != (reference & 0x7f);
}
