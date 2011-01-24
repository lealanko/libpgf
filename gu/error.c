#include "error.h"

struct GuError {
	GuPool* pool;
	void* data;
	GuType* type;
	const char* file;
	const char* func;
	int lineno;
};


GuError*
gu_error_new(GuPool* pool)
{
	GuError* err = gu_new(pool, GuError);
	err->pool = pool;
	err->data = NULL;
	err->type = NULL;
	err->file = NULL;
	err->func = NULL;
	err->lineno = -1;
	return err;
}

bool
gu_error_raised(GuError* err)
{
	return (err->type != NULL);
}

GuType*
gu_error_type(GuError* err)
{
	return err->type;
}

GuPool*
gu_error_pool(GuError* err)
{
	return err->pool;
}

void
gu_error_raise(GuError* err, 
	       GuType* type, void* data,
	       const char* filename, int lineno, const char* func)
{
	// TODO: sanity checking
	err->type = type;
	err->data = data;
	err->file = filename;
	err->func = func;
	err->lineno = lineno;
}
