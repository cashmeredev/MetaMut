/* { dg-do compile } */
/* { dg-options "-O2 -fsched2-use-superblocks -ftree-tail-merge" } */
/* { dg-require-effective-target alloca } */

typedef int aligned __attribute__ ((aligned (64)));
extern void abort (void);

void *p_1 = {0};
int bar (void *p);

void
foo (void)
{
  char *p = __builtin_alloca (-77);
  aligned i;

  if (bar (p) || bar (&i))
    abort ();
}
