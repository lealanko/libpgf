#ifndef GU_EXN_H_
#define GU_EXN_H_

#include <gu/mem.h>
#include <gu/type.h>

typedef struct GuExn GuExn;

typedef enum {
	GU_EXN_RAISED,
	GU_EXN_OK,
	GU_EXN_BLOCKED
} GuExnState;

struct GuExn {
	GuExnState state;
	GuExn* parent;
	GuKind* catch;
	GuPool* pool;
	GuType* caught;
	const void* data;
};



#define gu_exn(parent_, catch_, pool_) &(GuExn){	\
	.parent = parent_,	\
	.catch = gu_kind(catch_),	\
	.pool = pool_,		\
	.state = GU_EXN_OK,	\
	.caught = NULL,	\
	.data = NULL \
}

GuExn*
gu_new_exn(GuExn* parent, GuKind* catch_kind, GuPool* pool);

static inline bool
gu_exn_is_raised(GuExn* err) {
	return err && (err->state == GU_EXN_RAISED);
}

static inline void
gu_exn_clear(GuExn* err) {
	err->caught = NULL;
	err->state = GU_EXN_OK;
}

bool
gu_exn_is_raised(GuExn* err);

GuPool*
gu_exn_pool(GuExn* err, GuType* raise_type);

GuType*
gu_exn_caught(GuExn* err);

const void*
gu_exn_caught_data(GuExn* err);

void
gu_exn_block(GuExn* err);

void
gu_exn_unblock(GuExn* err);

GuExn*
gu_exn_raise_(GuExn* err, GuType* type);

GuExn*
gu_exn_raise_debug_(GuExn* err, GuType* type,
		      const char* filename, const char* func, int lineno);

#ifdef NDEBUG
#define gu_exn_raise(err_, type_)	\
	gu_exn_raise_(err_, type_)
#else
#define gu_exn_raise(err_, type_)	\
	gu_exn_raise_debug_(err_, type_, \
			      __FILE__, __func__, __LINE__)
#endif

#define gu_raise(error_, t_)		\
	gu_exn_raise(error_, gu_type(t_))

#define gu_raise_new(error_, t_, pool_, expr_)				\
	GU_BEGIN							\
	GuExn* gu_raise_err_ = gu_raise(error_, t_);			\
	if (gu_raise_err_) {							\
		GuPool* pool_ = gu_raise_err_->pool;			\
		gu_raise_err_->data = expr_;				\
	}								\
	GU_END

#define gu_raise_i(error_, t_, ...) \
	gu_raise_new(error_, t_, gu_raise_pool_, gu_new_i(gu_raise_pool_, t_, __VA_ARGS__))

static inline bool
gu_ok(GuExn* err) {
	return !GU_UNLIKELY(gu_exn_is_raised(err));
}

#define gu_return_on_exn(err_, retval_)		\
	GU_BEGIN					\
	if (gu_exn_is_raised(err_)) return retval_;	\
	GU_END


#include <errno.h>

typedef int GuErrno;

extern GU_DECLARE_TYPE(GuErrno, signed);

#define gu_raise_errno(error_) \
	gu_raise_i(error_, GuErrno, errno)

#include <stdio.h>

typedef void (*GuExnPrintFn)(GuFn* clo, void* err, FILE* out);

extern GuTypeTable gu_exn_default_printer;

void
gu_exn_print(GuExn* err, FILE* out, GuTypeMap printer_map);

#endif // GU_EXN_H_
