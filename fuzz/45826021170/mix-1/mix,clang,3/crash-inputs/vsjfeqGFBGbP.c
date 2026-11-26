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
  a[100000000000000000][({ int tmp_1; do { tmp_1 = (0); } while (({ int tmp_2; { tmp_2 = (0); } tmp_2; })); tmp_1; })] = b;
}

void
bar (void)
{
  a[100000000000000000][0].f = 1;
}
