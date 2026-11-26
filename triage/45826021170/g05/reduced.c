double bar() {
  _BitInt(843) i = bar();
  {
#pragma clang fp exceptions(strict)
    double tmp_1 = i;
  }
}
