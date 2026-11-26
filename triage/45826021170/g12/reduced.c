void foo();
void __attribute__bar() {
  char s[1099511627776];
  foo(s);
  s[0] = 'a';
}
