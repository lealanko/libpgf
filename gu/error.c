#include <gu/error.h>
#include <gu/assert.h>

struct GuError {
	GuPool* pool;
	GuErrorFrame* frame;
};

GuError*
gu_error_new(GuPool* pool)
{
	GuError* err = gu_new(pool, GuError);
	err->pool = pool;
	err->frame = NULL;
	return err;
}

bool
gu_error_raised(GuError* err)
{
	return (err->frame != NULL);
}

GuErrorFrame*
gu_error_frame(GuError* err)
{
	return err->frame;
}

GuType*
gu_error_type(GuError* err)
{
	return err->frame ? err->frame->type : NULL;
}

void*
gu_error_data(GuError* err)
{
	return err->frame ? err->frame->data : NULL;
}

GuPool*
gu_error_pool(GuError* err)
{
	return err->pool;
}

void
gu_error_raise(GuError* err, 
	       GuType* type, const void* data,
	       const char* filename, const char* func, int lineno)
{
	gu_assert(type);
	GuTypeRepr* repr = gu_type_repr(type);
	gu_assert(repr);
	void* err_data = gu_malloc_init_aligned(err->pool, 
						repr->size, repr->align,
						data);
	err->frame = gu_new_s(err->pool, GuErrorFrame,
			      .type = type,
			      .data = err_data,
			      .filename = filename,
			      .func = func,
			      .lineno = lineno,
			      .cause = err->frame);
}
