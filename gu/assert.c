#include <gu/assert.h>
#include <stdlib.h>
#include <stdio.h>

const char*
gu_assert_mode_descs[] = {
	[GU_ASSERT_PRECOND] = "precondition failed",
	[GU_ASSERT_POSTCOND] = "postcondition failed",
	[GU_ASSERT_ASSERTION] = "assertion failed",
	[GU_ASSERT_NEVER] = "control should not reach here",
};

void
gu_abort_(GuAssertMode mode, const char* msg, 
	  const char* file, const char* func, int line)
{
	const char* desc = gu_assert_mode_descs[mode];
	(void) fprintf(stderr, "%s (%s:%d): %s\n", func, file, line, desc);
	if (msg != NULL) {
		(void) fprintf(stderr, "\t%s\n", msg);
	}
	abort();
}
