/* PR middle-end/65680 */
/* { dg-do compile { target lp64 } } */

struct S
{
  int f : 1;
} a[100000000000000001][3];

void
foo (void)
{
  struct S b = { 0 };
  a[100000000000000000][(__builtin_choose_expr(sizeof(0) == 4, (0), (0)))] = b;
}

void
bar (void)
{
  a[100000000000000000][({ int tmp_1 = (0); if (tmp_1) { tmp_1 = tmp_1; } tmp_1; })].f = 1;
}
