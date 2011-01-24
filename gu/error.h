#ifndef GU_ERROR_H_
#define GU_ERROR_H_

#include <gu/mem.h>
#include <gu/type.h>

typedef struct GuError GuError;

GuError* gu_error_new(GuPool* pool);

bool gu_error_raised(GuError* err);

GuPool* gu_error_pool(GuError* err);

GuType* gu_error_type(GuError* err);

void* gu_error_value(GuError* err);

void gu_error_raise(GuError* err, 
		    GuType* type, void* data,
		    const char* filename, int lineno, const char* func);


#define gu_raise(err_, t_, init_) \
	GU_BEGIN				    \
	GuError* error_ = err_;			    \
	t_* p_ = gu_new(gu_error_pool(error_), t_); \
	t_ err_data_ = init_;			    \
	*p_ = err_data_;			    \
	gu_error_raise(error_, GU_TYPE(t_), p_, __FILE__, __LINE__, __func__); \
	GU_END


#include <stdio.h>

typedef struct GuErrorPrinter GuErrorPrinter;

struct GuErrorPrinter {
	void (*print)(void* ctx, GuError* err, FILE* out);
	void* ctx;
};

extern GuTypeTable gu_error_default_printer;

void gu_error_print(GuError* err, FILE* out, GuTypeMap printer_map);

#endif // GU_ERROR_H_
