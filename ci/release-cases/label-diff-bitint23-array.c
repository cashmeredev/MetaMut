#include <stdio.h>

int main(void) {
  static _BitInt(23) narrow[2] = {&&l2 - &&l1, 1};
  static int reference[2] = {&&l2 - &&l1, 1};
l1:
  puts("between the labels");
l2:
  printf("bitint=%d,%d int=%d,%d\n", (int)narrow[0], (int)narrow[1], reference[0], reference[1]);
  return (int)narrow[0] != reference[0] || (int)narrow[1] != reference[1];
}
