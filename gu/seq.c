#include <gu/seq.h>
#include <gu/fun.h>
#include <gu/assert.h>
#include <string.h>

struct GuSeq {
	uint8_t* buf;
	int buf_size;
	int n_elems;
	int head_idx;
	size_t elem_size;
};

static void
gu_seq_assert_invariant(GuSeq* seq)
{
	gu_assert(seq->elem_size > 0);
	gu_assert(seq->head_idx <= seq->buf_size);
	gu_assert(seq->n_elems <= seq->buf_size);
}

static void
gu_seq_free_fn(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	GuSeq* seq = clo->env1;
	gu_mem_buf_free(seq->buf);
}

GuSeq* 
gu_seq_new(GuPool* pool, size_t elem_size)
{
	gu_assert(elem_size > 0);
	GuSeq* seq = gu_new(pool, GuSeq);
	seq->n_elems = 0;
	seq->buf = NULL;
	seq->buf_size = 0;
	seq->elem_size = elem_size;
	seq->head_idx = 0;
	GuClo1* clo = gu_new_s(pool, GuClo1, gu_seq_free_fn, seq);
	gu_pool_finally(pool, &clo->fn);
	gu_seq_assert_invariant(seq);
	return seq;
}

int
gu_seq_size(const GuSeq* seq)
{
	return seq->n_elems;
}

static int
gu_seq_buf_index(GuSeq* seq, int idx)
{
	int i = (seq->head_idx + idx) % seq->buf_size;
	if (i < 0) {
		i += seq->buf_size;
	}
	return i;
}

void*
gu_seq_index(GuSeq* seq, int idx)
{
	gu_seq_assert_invariant(seq);
	if (idx < 0) {
		idx += seq->n_elems;
	}
	gu_assert(0 <= idx && idx < seq->n_elems);
	int buf_idx = gu_seq_buf_index(seq, idx);
	return &seq->buf[buf_idx * seq->elem_size];
}

void
gu_seq_resize_tail(GuSeq* seq, int change)
{
	gu_assert(change >= -seq->n_elems);
	int new_n_elems = seq->n_elems + change;
	if (new_n_elems > seq->buf_size) {
		size_t esz = seq->elem_size;
		int head_rev_idx = seq->buf_size - seq->head_idx;
		size_t allocation;
		uint8_t* new_buf = gu_mem_buf_realloc(seq->buf, 
						      new_n_elems * esz,
						      &allocation);
		gu_assert(new_buf != NULL);
		int new_buf_size = allocation / esz;
		int new_head_idx = new_buf_size - head_rev_idx;
		memmove(&new_buf[new_head_idx * esz], 
			&new_buf[seq->head_idx * esz], 
			head_rev_idx * esz);
		seq->buf = new_buf;	
		seq->buf_size = new_buf_size;
		seq->head_idx = new_head_idx;
	}
	// TODO: reallocate when shrinking enough
	seq->n_elems = new_n_elems;
}

void
gu_seq_resize_head(GuSeq* seq, int change)
{
	if (change < 0) {
		gu_assert(-change <= seq->n_elems);
		seq->head_idx = gu_seq_buf_index(seq, -change);
	}
	gu_seq_resize_tail(seq, change);
	if (change > 0) {
		seq->head_idx = gu_seq_buf_index(seq, -change);
	}
}

#include <gu/type.h>

GU_DEFINE_KIND(GuSeq, abstract);

GU_DEFINE_TYPE(GuCharSeq, GuSeq, gu_type(char));
GU_DEFINE_TYPE(GuByteSeq, GuSeq, gu_type(uint8_t));


GuString*
gu_char_seq_to_string(GuCharSeq* charq, GuPool* pool)
{
	int size = gu_char_seq_size(charq);
	GuString* string = gu_string_new(pool, size);
	char* data = gu_string_data(string);
	for (int i = 0; i < size; i++) {
		data[i] = *gu_char_seq_index(charq, i);
	}
	return string;
}
