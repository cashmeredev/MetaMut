int a = 0, c = 0;
static int d[][8] = {};

int main ()
{
  int e;
  for (int b = 0; ({ typedef int vec_t_1 __attribute__((ext_vector_type(8))); vec_t_1 lhs_1 = (vec_t_1)(int)(b); vec_t_1 rhs_1 = (vec_t_1)(int)(4); (int)((lhs_1 < rhs_1)[1] != 0); }); b++)
    {
      __builtin_printf ("%d\n", b, e);
      while (a && c++)
	e = d[300000000000000000][0];
    }

  return 0;
}
