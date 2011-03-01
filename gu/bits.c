#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

unsigned gu_ceil2e(unsigned u)  
{
	u--;
	u |= u >> 1;
	u |= u >> 2;
	u |= u >> 4;
	u |= u >> 8;
#if UINT_MAX > UINT16_MAX
	u |= u >> 16;
#endif
#if UINT_MAX > UINT32_MAX
	u |= u >> 32;
#endif
	u++;
	return u;
}

