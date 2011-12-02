#include <gu/error.h>
#include <gu/assert.h>


GuError*
gu_new_error(GuError* parent, GuKind* catch, GuPool* pool)
{
	return gu_new_s(pool, GuError,
			.parent = parent,
			.catch = catch,
			.pool = pool,
			.state = GU_ERROR_OK,
			.caught = NULL,
			.data = NULL);
}

void
gu_error_block(GuError* err)
{
	if (err && err->state == GU_ERROR_RAISED) {
		err->state = GU_ERROR_BLOCKED;
	}
}

void
gu_error_unblock(GuError* err)
{
	if (err && err->state == GU_ERROR_BLOCKED) {
		err->state = GU_ERROR_RAISED;
	}
}

GuError*
gu_error_raise_debug_(GuError* base, GuType* type, 
		      const char* filename, const char* func, int lineno)
{
	gu_require(type);

	// TODO: log the error, once there's a system for dumping
	// error objects.

	GuError* err = base;
	
	while (err && !(err->catch && gu_type_has_kind(type, err->catch))) {
		err->state = GU_ERROR_RAISED;
		err = err->parent;
	}
	if (!err) {
		gu_abort_(GU_ASSERT_ASSERTION, filename, func, lineno,
			  "Unexpected error raised");
	}

	if (err->state == GU_ERROR_OK) {
		err->caught = type;
		err->state = GU_ERROR_RAISED;
		return err;
	} 
	err->state = GU_ERROR_RAISED;
	return NULL;
}

GuError*
gu_error_raise_(GuError* base, GuType* type)
{
	return gu_error_raise_debug_(base, type, NULL, NULL, -1);
}


GU_DEFINE_TYPE(GuErrno, int, _);
