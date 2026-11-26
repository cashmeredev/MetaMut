double bar(void), c;
int foo(void) {
	double a, b;
	_BitInt(843) i = bar() + bar();
	a = i; i += 1; a += 0.1; i = c + ({
#pragma clang fp exceptions(strict)
 double tmp_1 = (i); if (tmp_1) { tmp_1 = tmp_1; } tmp_1; });
	return i;
}
