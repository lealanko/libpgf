#ifndef GU_ERROR_H_
#define GU_ERROR_H_

#include <gu/mem.h>
#include <gu/type.h>

typedef const struct GuErrorFrame GuErrorFrame;

struct GuErrorFrame {
	GuType* type;
	void* data;
	const char* filename;
	int lineno;
	const char* func;
	GuErrorFrame* cause;
};

typedef struct GuError GuError;

GuError*
gu_error_new(GuPool* pool);

bool
gu_error_raised(GuError* err);

GuPool*
gu_error_pool(GuError* err);

GuType*
gu_error_type(GuError* err);

void*
gu_error_data(GuError* err);

GuErrorFrame*
gu_error_frame(GuError* err);

void
gu_error_raise(GuError* err, GuType* type, const void* data,
	       const char* filename, const char* func, int lineno);

#define gu_raise(error_, t_, ...) \
	gu_error_raise(error_, gu_type(t_), &(t_) { __VA_ARGS__ }, \
		       __FILE__, __func__, __LINE__)

static inline bool
gu_ok(GuError* err) {
	return !gu_error_raised(err);
}

#define gu_return_on_error(err_, retval_)		\
	GU_BEGIN					\
	if (gu_error_raised(err_)) return retval_;	\
	GU_END


#include <stdio.h>

typedef void (*GuErrorPrintFn)(GuFn* clo, void* err, FILE* out);

extern GuTypeTable gu_error_default_printer;

void
gu_error_print(GuError* err, FILE* out, GuTypeMap printer_map);

#endif // GU_ERROR_H_
