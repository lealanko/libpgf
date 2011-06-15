#include <stdio.h>
#include <gu/seq.h>
#include <gu/dump.h>

GU_SEQ_DEFINE(Ints, ints, int);

static GU_DEFINE_TYPE(Ints, GuSeq, gu_type(int));


int main(void)
{
	GuPool* pool = gu_pool_new();
	Ints* is = ints_new(pool);
	for (int i = 0; i < 100; i++) {
		ints_push(is, i * i);
		ints_push(is, i * i + 1);
		printf("shift: %d\n", ints_shift(is));
	}
	for (int i = 0; i < ints_size(is); i++) {
		printf("%d: %d\n", i, *ints_index(is, i));
	}
	GuDumpCtx* ctx = gu_dump_ctx_new(pool, stdout, NULL);
	gu_dump(gu_type(Ints), is, ctx);

	while (ints_size(is) > 0) {
		printf("pop: %d\n", ints_pop(is));
	}
	gu_pool_free(pool);
	return 0;
}
