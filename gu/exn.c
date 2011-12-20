#include <gu/exn.h>
#include <gu/assert.h>


GuExn*
gu_new_exn(GuExn* parent, GuKind* catch, GuPool* pool)
{
	return gu_new_s(pool, GuExn,
			.parent = parent,
			.catch = catch,
			.pool = pool,
			.state = GU_EXN_OK,
			.caught = NULL,
			.data = NULL);
}

void
gu_exn_block(GuExn* err)
{
	if (err && err->state == GU_EXN_RAISED) {
		err->state = GU_EXN_BLOCKED;
	}
}

void
gu_exn_unblock(GuExn* err)
{
	if (err && err->state == GU_EXN_BLOCKED) {
		err->state = GU_EXN_RAISED;
	}
}

GuExn*
gu_exn_raise_debug_(GuExn* base, GuType* type, 
		      const char* filename, const char* func, int lineno)
{
	gu_require(type);

	// TODO: log the error, once there's a system for dumping
	// error objects.

	GuExn* err = base;
	
	while (err && !(err->catch && gu_type_has_kind(type, err->catch))) {
		err->state = GU_EXN_RAISED;
		err = err->parent;
	}
	if (!err) {
		gu_abort_(GU_ASSERT_ASSERTION, filename, func, lineno,
			  "Unexpected error raised");
	}

	if (err->state == GU_EXN_OK) {
		err->caught = type;
		err->state = GU_EXN_RAISED;
		return err;
	} 
	err->state = GU_EXN_RAISED;
	return NULL;
}

GuExn*
gu_exn_raise_(GuExn* base, GuType* type)
{
	return gu_exn_raise_debug_(base, type, NULL, NULL, -1);
}


GU_DEFINE_TYPE(GuErrno, signed, _);
