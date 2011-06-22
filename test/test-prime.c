#include <gu/prime.h>
#include <gu/assert.h>
#include <stdio.h>

bool is_prime(int i)
{
	for (int j = 2; j * j <= i; j++) {
		if (i % j == 0) {
			return false;
		}
	}
	return true;
}

void assert_inf_prime(int p, int j) {
	gu_assert(is_prime(p));
	gu_assert(p <= j);
	for (int k = p + 1; k <= j; k++) {
		gu_assert(!is_prime(k));
	}
}

void assert_sup_prime(int i, int p) {
	gu_assert(is_prime(p));
	gu_assert(i <= p);
	for (int k = p - 1; k >= i; k--) {
		gu_assert(!is_prime(k));
	}
}

int main(void)
{
	for (int i = 3; i < 10000; i++) {
		int p = gu_prime_inf(i);
		assert_inf_prime(p, i);
		int sp = gu_prime_sup(i);
		assert_sup_prime(i, sp);
		gu_assert(is_prime(i) == gu_is_prime(i));
	}
	for (int i = 0; i < 1000; i++) {
		int p = gu_prime_sup(i);
		printf("%d\n", p);
		i = p;
	}
	return 0;
}
