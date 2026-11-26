/* PR middle-end/65680 */
/* { dg-do compile { target lp64 } } */

struct S
{
  int f : 1;
} a[100000000000000001][3];

void
foo (void)
{
  struct S b = { ((int)__builtin_rotateleft16((__UINT16_TYPE__)(0), 23)) };
  a[100000000000000000][0] = b;
}

void
bar (void)
{
  a[100000000000000000][0].f = 1;
}
