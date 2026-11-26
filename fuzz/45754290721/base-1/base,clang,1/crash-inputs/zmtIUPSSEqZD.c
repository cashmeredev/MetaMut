/* { dg-do link } */
/* { dg-require-alias "" } */

inline int bar () __attribute__ ((alias ("foo")));
void foo () { }
int main () { return 0; }
