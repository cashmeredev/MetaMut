#include <stdarg.h>
struct S {
  float a[1804289383]
} check() {
  va_list ap;
  va_arg(ap, struct S);
}
