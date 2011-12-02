#ifndef GU_ERROR_H_
#define GU_ERROR_H_

#include <gu/mem.h>
#include <gu/type.h>

typedef struct GuError GuError;

typedef enum {
	GU_ERROR_RAISED,
	GU_ERROR_OK,
	GU_ERROR_BLOCKED
} GuErrorState;

struct GuError {
	GuError* parent;
	GuKind* catch;
	GuPool* pool;
	GuErrorState state;
	GuType* caught;
	const void* data;
};



#define gu_error(parent_, catch_, pool_) &(GuError){	\
	.parent = parent_,	\
	.catch = gu_kind(catch_),	\
	.pool = pool_,		\
	.state = GU_ERROR_OK,	\
	.caught = NULL,	\
	.data = NULL \
}

GuError*
gu_new_error(GuError* parent, GuKind* catch_kind, GuPool* pool);

static inline bool
gu_error_is_raised(GuError* err) {
	return err && (err->state == GU_ERROR_RAISED);
}

static inline void
gu_error_clear(GuError* err) {
	err->caught = NULL;
	err->state = GU_ERROR_OK;
}

bool
gu_error_is_raised(GuError* err);

GuPool*
gu_error_pool(GuError* err, GuType* raise_type);

GuType*
gu_error_caught(GuError* err);

const void*
gu_error_caught_data(GuError* err);

void
gu_error_block(GuError* err);

void
gu_error_unblock(GuError* err);

GuError*
gu_error_raise_(GuError* err, GuType* type);

GuError*
gu_error_raise_debug_(GuError* err, GuType* type,
		      const char* filename, const char* func, int lineno);

#ifdef NDEBUG
#define gu_error_raise(err_, type_)	\
	gu_error_raise_(err_, type_)
#else
#define gu_error_raise(err_, type_)	\
	gu_error_raise_debug_(err_, type_, \
			      __FILE__, __func__, __LINE__)
#endif

#define gu_raise(error_, t_)		\
	gu_error_raise(error_, gu_type(t_))

#define gu_raise_new(error_, t_, pool_, expr_)				\
	GU_BEGIN							\
	GuError* gu_raise_err_ = gu_raise(error_, t_);			\
	if (gu_raise_err_) {							\
		GuPool* pool_ = gu_raise_err_->pool;			\
		gu_raise_err_->data = expr_;				\
	}								\
	GU_END

#define gu_raise_i(error_, t_, ...) \
	gu_raise_new(error_, t_, gu_raise_pool_, gu_new_i(gu_raise_pool_, t_, __VA_ARGS__))

static inline bool
gu_ok(GuError* err) {
	return !gu_error_is_raised(err);
}

#define gu_return_on_error(err_, retval_)		\
	GU_BEGIN					\
	if (gu_error_is_raised(err_)) return retval_;	\
	GU_END


#include <errno.h>

typedef int GuErrno;

extern GU_DECLARE_TYPE(GuErrno, signed);

#define gu_raise_errno(error_) \
	gu_raise_i(error_, GuErrno, errno)

#include <stdio.h>

typedef void (*GuErrorPrintFn)(GuFn* clo, void* err, FILE* out);

extern GuTypeTable gu_error_default_printer;

void
gu_error_print(GuError* err, FILE* out, GuTypeMap printer_map);

#endif // GU_ERROR_H_
