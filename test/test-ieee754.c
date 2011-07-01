#include <limits.h>
#include <gu/bits.h>
#include <float.h>
#include <math.h>


int main(void) {
	double d[] = {
		0.0,
		nextafter(0.0, 1.0),
		nextafter(DBL_MIN, 0.0),
		DBL_MIN,
		nextafter(DBL_MIN, 1.0),
		nextafter(1.0, 0.0),
		1.0,
		nextafter(1.0, 2.0),
		2.0,
		3.14159265358979323846,
		INFINITY,
		NAN,
		nan("34567")
	};
	for (int i = 0; i < (int)(sizeof(d)/sizeof(d[0])); i++) {
		uint64_t u;
		memcpy(&u, &d[i], sizeof(double));
		double x = gu_decode_double(u);
		gu_assert(memcmp(&x, &u, sizeof(double)) == 0);
		double t = -d[i];
		memcpy(&u, &t, sizeof(double));
		x = gu_decode_double(u);
		gu_assert(memcmp(&x, &u, sizeof(double)) == 0);
	}
	return 0;
}
