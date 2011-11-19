#ifndef GU_SEQ_H_
#define GU_SEQ_H_

#include <gu/mem.h>
#include <gu/bits.h>
#include <gu/type.h>


typedef GuTagged() GuSeq;

GuSeq
gu_seq_new(GuPool* pool);

GuSeq
gu_seq_new_sized(size_t align, size_t size, GuPool* pool);

static inline int
gu_seq_size(GuSeq seq)
{
	GuWord w = seq.w_;
	int tag = gu_word_tag(w);
	if (tag == 0) {
		GuWord* p = gu_word_ptr(w);
		return (int) (p[-1] >> 1);
	}
	return tag;
}

static inline void*
gu_seq_data(GuSeq seq)
{
	GuWord w = seq.w_;
	int tag = gu_word_tag(w);
	void* ptr = gu_word_ptr(w);
	if (tag == 0) {
		GuWord* p = ptr;
		if (p[-1] & 0x1) {
		 	return *(uint8_t**) ptr;
		}
	}
	return ptr;
}

static inline bool
gu_seq_is_null(GuSeq seq)
{
	return (gu_word_ptr(seq.w_)) == NULL;
}

static inline void*
gu_seq_index(GuSeq seq, int i)
{
	gu_require(i < gu_seq_size(seq));
	uint8_t* data = gu_seq_data(seq);
	return &data[i];
}

void
gu_seq_resize_tail(GuSeq seq, int change);

void
gu_seq_push(GuSeq seq, const void* data, size_t size);

void
gu_seq_pop(GuSeq seq, size_t size, void* data_out);

void
gu_seq_resize_head(GuSeq seq, int change);

void
gu_seq_unshift(GuSeq seq, const void* data, size_t size);

void
gu_seq_shift(GuSeq seq, size_t size, void* data_out);

#define GU_SEQ_NULL { .w_ = (uintptr_t) NULL }

GU_DECLARE_KIND(GuSeq);

struct GuSeqType {
	GuType_abstract abstract_base;
	GuType* elem_type;
};

typedef const struct GuSeqType GuSeqType, GuType_GuSeq;

#define GU_TYPE_INIT_GuSeq(k_, t_, elem_type_) {	   \
	.abstract_base = GU_TYPE_INIT_abstract(k_, t_, _), \
	.elem_type = elem_type_,		   	\
}



#define GU_SEQ_DEFINE(t_, pfx_, elem_t_)				\
									\
	typedef struct { GuSeq seq_; } t_;				\
									\
	static inline t_						\
	pfx_##_new(GuPool* pool)					\
	{								\
		return (t_) { gu_seq_new(pool) };			\
	}								\
									\
	static inline t_						\
	pfx_##_new_sized(int size, GuPool* pool)			\
	{								\
		return (t_) { gu_seq_new_sized(gu_alignof(t_),		\
					       size * sizeof(elem_t_),	\
					       pool) };			\
	}								\
									\
	static inline int						\
	pfx_##_size(t_ s)						\
	{								\
		return gu_seq_size(s.seq_) / sizeof(elem_t_);		\
	}								\
									\
	static inline elem_t_*						\
	pfx_##_data(t_ s)						\
	{								\
		return gu_seq_data(s.seq_);				\
	}								\
									\
	static inline bool						\
	pfx_##_is_null(t_ s)						\
	{								\
		return gu_seq_is_null(s.seq_);				\
	}								\
									\
	static inline elem_t_*						\
	pfx_##_index(t_ s, int idx)					\
	{								\
		return &pfx_##_data(s)[idx];				\
	}								\
									\
	static inline elem_t_						\
	pfx_##_get(t_ s, int idx)					\
	{								\
		return *pfx_##_index(s, idx);				\
	}								\
									\
	static inline void						\
	pfx_##_set(t_ s, int idx, elem_t_ v)				\
	{								\
		*pfx_##_index(s, idx) = v;				\
	}								\
									\
	static inline void						\
	pfx_##_push_n(t_ s, elem_t_ const* elems, size_t n)		\
	{								\
		gu_seq_push(s.seq_, elems, n * sizeof(elem_t_));	\
	}								\
									\
	static inline void						\
	pfx_##_push(t_ s, elem_t_ elem)					\
	{								\
		pfx_##_push_n(s, &elem, 1);				\
	}								\
									\
	static inline elem_t_						\
	pfx_##_pop(t_ s)						\
	{								\
		elem_t_ ret;						\
		gu_seq_pop(s.seq_, sizeof(elem_t_), &ret);		\
		return ret;						\
	}								\
									\
	static inline void						\
	pfx_##_unshift(t_ s, elem_t_ elem)				\
	{								\
		gu_seq_unshift(s.seq_, &elem, sizeof(elem_t_));		\
	}								\
									\
	static inline elem_t_						\
	pfx_##_shift(t_ s)						\
	{								\
		elem_t_ ret;						\
		gu_seq_shift(s.seq_, sizeof(elem_t_), &ret);		\
		return ret;						\
	}								\
									\
	GU_DECLARE_DUMMY

GU_SEQ_DEFINE(GuCharSeq, gu_char_seq, char);
GU_DECLARE_TYPE(GuCharSeq, GuSeq);
GU_SEQ_DEFINE(GuByteSeq, gu_byte_seq, uint8_t);
GU_DECLARE_TYPE(GuByteSeq, GuSeq);
GU_SEQ_DEFINE(GuWcSeq, gu_wc_seq, wchar_t);
GU_DECLARE_TYPE(GuWcSeq, GuSeq);

char*
gu_char_seq_to_str(GuCharSeq charq, GuPool* pool);

#include <wchar.h>

wchar_t*
gu_wc_seq_wcs(GuWcSeq* wcq, GuPool* pool);


#endif // GU_SEQ_H_
