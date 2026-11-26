double foo_b;
void foo() {
#pragma STDC FENV_ACCESS ON
  _BitInt(692) i = foo_b * i;
}
