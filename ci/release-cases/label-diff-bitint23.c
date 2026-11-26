#include <stdio.h>

int main(void) {
  static _BitInt(23) narrow = &&l2 - &&l1;
  static int reference = &&l2 - &&l1;
l1:
  puts("between the labels");
l2:
  printf("bitint=%d int=%d\n", (int)narrow, reference);
  return (int)narrow != reference;
}
