int foo6_w;
void foo6() {
  asm goto("" : "+avL"(foo6_w) : : : lab5);
lab5:
}
