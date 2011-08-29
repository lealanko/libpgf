#ifndef GU_SEQ_H_
#define GU_SEQ_H_

#include <gu/mem.h>

typedef struct GuSeq GuSeq;

GuSeq*
gu_seq_new(GuPool* pool);

GuSeq*
gu_seq_new_fixed(int size, GuPool* pool);

int
gu_seq_size(const GuSeq* seq);

void*
gu_seq_index(GuSeq* seq, int idx);

void
gu_seq_resize_head(GuSeq* seq, int change);

void
gu_seq_resize_tail(GuSeq* seq, int change);

#define GU_SEQ_DEFINE(t_, pfx_, elem_t_)				\
									\
	typedef struct t_ t_;						\
									\
	static inline t_*						\
	pfx_##_new(GuPool* pool)					\
	{								\
		t_* d_ = (t_*) gu_seq_new(pool);			\
		return d_;						\
	}								\
									\
	static inline t_*						\
	pfx_##_new_fixed(int size, GuPool* pool)			\
	{								\
		t_* d_ = (t_*) gu_seq_new_fixed(size * sizeof(elem_t_), \
						pool);			\
		return d_;						\
	}								\
									\
	static inline int						\
	pfx_##_size(const t_* d_)					\
	{								\
		return gu_seq_size((const GuSeq*) d_) / sizeof(elem_t_); \
	}								\
									\
	static inline elem_t_*						\
	pfx_##_index(t_* d_, int idx)					\
	{								\
		return gu_seq_index((GuSeq*) d_, idx * sizeof(elem_t_)); \
	}								\
									\
	static inline const elem_t_*					\
	pfx_##_cindex(const t_* d_, int idx)				\
	{								\
		return pfx_##_index((t_*) d_, idx);			\
	}								\
									\
	static inline elem_t_						\
	pfx_##_get(const t_* d_, int idx)				\
	{								\
		return *pfx_##_cindex(d_, idx);				\
	}								\
									\
	static inline void						\
	pfx_##_set(t_* d_, int idx, elem_t_ v_)				\
	{								\
		*pfx_##_index(d_, idx) = v_;				\
	}								\
									\
	static inline void						\
	pfx_##_push(t_* d_, elem_t_ elem)				\
	{								\
		gu_seq_resize_tail((GuSeq*) d_, sizeof(elem_t_));	\
		*pfx_##_index(d_, -1) = elem;				\
	}								\
									\
	static inline elem_t_						\
	pfx_##_pop(t_* d_)						\
	{								\
		elem_t_ ret = *pfx_##_index(d_, -1);			\
		gu_seq_resize_tail((GuSeq*) d_, -(int)sizeof(elem_t_));	\
		return ret;						\
	}								\
									\
	static inline void						\
	pfx_##_unshift(t_* d_, elem_t_ elem)				\
	{								\
		gu_seq_resize_head((GuSeq*) d_, sizeof(elem_t_));	\
		*pfx_##_index(d_, 0) = elem;				\
	}								\
									\
	static inline elem_t_						\
	pfx_##_shift(t_* d_)						\
	{								\
		elem_t_ ret = pfx_##_get(d_, 0);			\
		gu_seq_resize_head((GuSeq*) d_, -(int)sizeof(elem_t_));	\
		return ret;						\
	}								\
									\
	GU_DECLARE_DUMMY



#include <gu/type.h>

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

GU_SEQ_DEFINE(GuCharSeq, gu_char_seq, char);
GU_DECLARE_TYPE(GuCharSeq, GuSeq);

GU_SEQ_DEFINE(GuByteSeq, gu_byte_seq, uint8_t);
GU_DECLARE_TYPE(GuByteSeq, GuSeq);

#include <gu/string.h>

GuString*
gu_char_seq_to_string(GuCharSeq* charq, GuPool* pool);

#endif // GU_SEQ_H_
