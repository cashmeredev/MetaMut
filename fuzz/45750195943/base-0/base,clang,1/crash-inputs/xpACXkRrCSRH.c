typedef short typedef_t_1;
/* PR tree-optimization/105094 */
/* { dg-do compile } */
/* { dg-options "-O2" } */

struct S { typedef_t_1 a; char b[~(__SIZE_TYPE__)0 / __CHAR_BIT__ - 1]; };
void bar (struct S *);

void
foo (void)
{
  struct S s = { 5 };
  bar (&s);
}
