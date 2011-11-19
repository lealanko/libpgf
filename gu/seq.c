#include <gu/seq.h>
#include <gu/fun.h>
#include <gu/assert.h>
#include <string.h>


typedef struct GuDynSeq GuDynSeq;

struct GuDynSeq {
	uint8_t* buf;
	int head_idx;
	size_t buf_size;
	GuFinalizer fin;
};

static GuDynSeq*
gu_seq_dyn(GuSeq seq)
{
	gu_assert(gu_word_tag(seq.w_) == 0);
	return (void*) seq.w_;
}

static int
gu_dyn_seq_size(GuDynSeq* dyn)
{
	return (int)(((GuWord*)(void*)dyn)[-1] >> 1);
}

static void
gu_dyn_seq_set_size(GuDynSeq* dyn, int new_size)
{
	((GuWord*)(void*)dyn)[-1] = ((GuWord) new_size) << 1 | 0x1;
}

static void
gu_seq_free_fn(GuFinalizer* fin)
{
	GuDynSeq* dyn = gu_container(fin, GuDynSeq, fin);
	gu_mem_buf_free(dyn->buf);
}

GuSeq
gu_seq_new(GuPool* pool)
{
	GuDynSeq* dyn = gu_new_prefixed(pool, unsigned, GuDynSeq);
	gu_dyn_seq_set_size(dyn, 0);
	dyn->buf = NULL;
	dyn->buf_size = 0;
	dyn->head_idx = 0;
	dyn->fin.fn = gu_seq_free_fn;
	gu_pool_finally(pool, &dyn->fin);
	gu_dyn_seq_set_size(dyn, 0);
	return (GuSeq) { gu_word(dyn, 0) };
}

GuSeq
gu_seq_new_sized(size_t align, size_t size, GuPool* pool)
{
	if (0 < size && size <= (size_t) gu_word_max_tag()) {
		void* buf = gu_malloc_aligned(pool, size, align);
		return (GuSeq) { gu_word(buf, (int) size) };
	} else {
		void* buf = gu_malloc_prefixed(pool,
					       gu_alignof(GuWord), 
					       sizeof(GuWord),
					       align, size);
		((GuWord*) buf)[-1] = ((GuWord) size) << 1;
		return (GuSeq) { gu_word(buf, 0) };
	}
}

static void
gu_dyn_seq_resize_tail(GuDynSeq* dyn, int change)
{
	int size = gu_dyn_seq_size(dyn);
	gu_require(size + change >= 0);
	int new_size = size + change;
	if ((size_t) new_size > dyn->buf_size) {
		dyn->buf = gu_mem_buf_realloc(dyn->buf, new_size,
					      &dyn->buf_size);
	}
	gu_dyn_seq_set_size(dyn, new_size);
	gu_ensure((size_t) size <= dyn->buf_size);
}

static void
gu_dyn_seq_push(GuDynSeq* dyn, const void* data, size_t size)
{
	int old_size = gu_dyn_seq_size(dyn);
	gu_dyn_seq_resize_tail(dyn, (int) size);
	memcpy(&dyn->buf[old_size], data, size);
}


static void
gu_dyn_seq_pop(GuDynSeq* dyn, size_t size, void* data_out)
{
	int old_size = gu_dyn_seq_size(dyn);
	gu_require((int) size <= old_size);
	memcpy(data_out, &dyn->buf[old_size - (int) size], size);
	gu_dyn_seq_resize_tail(dyn, -(int) size);
}

void
gu_seq_pop(GuSeq seq, size_t size, void* data_out)
{
	gu_dyn_seq_pop(gu_seq_dyn(seq), size, data_out);
}

void
gu_seq_push(GuSeq seq, const void* data, size_t size)
{
	gu_dyn_seq_push(gu_seq_dyn(seq), data, size);
}

#include <gu/type.h>

GU_DEFINE_KIND(GuSeq, abstract);

GU_DEFINE_TYPE(GuCharSeq, GuSeq, gu_type(char));
GU_DEFINE_TYPE(GuByteSeq, GuSeq, gu_type(uint8_t));

char*
gu_char_seq_to_str(GuCharSeq charq, GuPool* pool)
{
	int size = gu_char_seq_size(charq);
	char* data = gu_char_seq_data(charq);
	char* str = gu_str_new(size, pool);
	memcpy(str, data, size);
	return str;
}
