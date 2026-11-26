typedef int typedef_t_1;
/* PR29970, PR91038 */
/* { dg-do run } */
/* { dg-options "-O2 -Wunused-variable" } */

typedef_t_1 foo3b(void)   // should not return 0
{
        typedef_t_1 n = 0;
        return sizeof *({ n = 10; typedef_t_1 x[n]; &x; });
}

typedef_t_1 foo4(void)   // should not ICE
{
        return (*({
                        typedef_t_1 n = 20;
                        char (*x)[n][n] = __builtin_malloc(n * n);
                        (*x)[12][1] = 1;
                        x;
                }))[12][1];
}

typedef_t_1 foo5(void)   // should return 1, returns 0
{
        typedef_t_1 n = 0;
        return (*({
                        n = 20;
                        char (*x)[n][n] = __builtin_malloc(n * n);
                        (*x)[12][1] = 1;
                        (*x)[0][1] = 0;
                        x;
                }))[12][1];
}

typedef_t_1 foo5c(void)   // should return 400 
{
        typedef_t_1 n = 0;
        return sizeof(*({
                        n = 20;
                        char (*x)[n][n] = __builtin_malloc(n * n);
                        (*x)[12][1] = 1;
                        (*x)[0][1] = 0;
                        x;
                }));
}

typedef_t_1 foo5b(void)   // should return 1, returns 0
{
	typedef_t_1 n = 0;	/* { dg-warning "unused variable" } */
        return (*({
                        typedef_t_1 n = 20;
                        char (*x)[n][n] = __builtin_malloc(n * n);
                        (*x)[12][1] = 1;
                        (*x)[0][1] = 0;
                        x;
                }))[12][1];
}

typedef_t_1 foo5a(void)   // should return 1, returns 0
{
        return (*({
                        typedef_t_1 n = 20;
                        char (*x)[n][n] = __builtin_malloc(n * n);
                        (*x)[12][1] = 1;
                        (*x)[0][1] = 0;
                        x;
                }))[12][1];
}




typedef_t_1 main()
{
	if (sizeof(typedef_t_1[10]) != foo3b())
		__builtin_abort();

	if (1 != foo4())
		__builtin_abort();

	if (400 != foo5c())
		__builtin_abort();

	if (1 != foo5a())
		__builtin_abort();

	if (1 != foo5b()) // -O0
		__builtin_abort();

	if (1 != foo5())
		__builtin_abort();

	return 0;
}


