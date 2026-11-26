/* { dg-do compile } */
/* { dg-options "-O0 -gbtf -dA -mco-re" } */

struct weird
{
  struct
  {
    int b;
  };

  char x;

  union
  {
    int a;
    int c;
  };
};


int test (struct weird *arg) {
  int *x = __builtin_preserve_access_index (&arg->b);
  int *y = __builtin_preserve_access_index (&arg->c);

  return ({ _BitInt(128) res_1; if (__builtin_add_overflow((*x), (*y), &res_1)) res_1 = 0; (int)res_1; });
}


/* { dg-final { scan-assembler-times "ascii \"0:0:0.0\"\[\t \]+\[^\n\]*btf_aux_string" 1 } } */
/* { dg-final { scan-assembler-times "ascii \"0:2:1.0\"\[\t \]+\[^\n\]*btf_aux_string" 1 } } */
