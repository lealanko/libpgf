#ifndef GU_EXN_H_
#define GU_EXN_H_

#include <gu/mem.h>
#include <gu/type.h>

/** @file
 *
 * Exceptions.
 *
 * Since C has no built-in exception machinery, `libgu` uses the following
 * framework for error reporting, propagation and handling. The framework is
 * mainly concerned with allocating exception data and dispatching handlers.
 * It does not automatically transfer control from the raiser of the exception
 * to the handler, so all code that uses exceptions must manually check if an
 * exception has been raised.
 *
 * In this framework, an "exception" is composed of a _tag_, i.e. a #GuType
 * representing some type, and optional _data_, i.e. an object of that type.
 * Each handler catches all exceptions whose tags belong to a particular
 * #GuKind.
 *  
 */


/** @name Creating exception frames
 *
 * Transmission of exceptions happens with _exception frames_ of type #GuExn.
 * Each exception frame handles a particular kind of exception, and multiple
 * handlers can be chained together. When an exception is raised, the data
 * associated with the exception is stored in the nearest frame that handles
 * that kind of exception. Each frame is associated with a memory pool
 * (#GuPool) from which the space used to store the exception data is
 * allocated. Since this pool lives at least as long as the exception frame,
 * the exception data is guaranteed to be alive when the handler is run.
 *
 * Typically, an exception frame is created with #gu_exn and lives until the
 * end of the block surrounding the #gu_exn invocation. For special purposes,
 * the more general #gu_new_exn can be used.
 * 
 */



typedef struct GuExn GuExn;

/**< An exception frame. */






/// Allocate a new local exception frame.
#define gu_exn(parent, CATCH_KIND, pool) &(GuExn){	\
	.state = GU_EXN_OK,	\
	.parent = (parent),	\
	.catch = gu_kind(CATCH_KIND),	\
	.caught = NULL,	\
	.data.pool = (pool),	\
	.data.data = NULL \
}

/**<
 * @param parent      #GuExn `*`
 * @param CATCH_KIND  a kind name
 * @param pool        #GuPool `*`
 */


/// Allocate a new exception frame.
GuExn*
gu_new_exn(GuExn* parent, GuKind* catch_kind, GuPool* pool);

/**< Creates a new exception frame that catches exceptions of kind
 * `catch_kind` and propagates other exceptions to `parent`. The exception
 * frame and the data of caught exceptions is allocated from `pool`.
 */


/** @name Raising exceptions
 *
 * Raising an exception happens in two stages: first the exception is propated
 * from the current frame to the frame that handles the exception. All frames
 * in the path are marked as raised. Then the exception data, if any, is
 * allocated from the handler frame's pool and stored in the frame. This way the
 * lifetime of the exception data is exactly as long as required by the
 * handler.
 *
 * If the exception data is a single object, #gu_raise_i is a convenient way
 * to perform both these stages at once. If there is no exception data, or
 * multiple objects need to be allocated, then lower-level #gu_raise operation
 * can be called, and the returned #GuExnData structured used for allocating
 * and storing the data.
 *
 */

/// A structure for storing exception values.
typedef struct GuExnData GuExnData;

/// A structure for storing exception values.
struct GuExnData
/**< When an exception is raised, if there is an associated value, it
 * must be allocated from a pool that still exists when control
 * returns to the handler of that exception. This structure is used to
 * communicate the exception from the raiser to the handler: the
 * handler sets #pool when setting up the exception frame, and the
 * raiser uses that pool to allocate the value and stores that in
 * #data. When control returns to the handler, it reads the value from
 * the #data member.
 */
{
	
	/// The pool that the exception value should be allocated from.
	GuPool* const pool;

	/// The exception value.
	const void* data;
};



/// @private
GuExnData*
gu_exn_raise_(GuExn* err, GuType* type);

/// @private
GuExnData*
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

/// Raise an exception.
#define gu_raise(exn, T)		\
	gu_exn_raise(exn, gu_type(T))
/**< 
 * @param exn The current exception frame.
 *
 * @param T   The C type of the exception to raise. There must be an associated
 *            #GuType defined and in scope.
 *
 * @return
 *
 * A #GuExnData object that can be used to allocate and store the exception
 * data, or `NULL` if the exception will be ignored and no data is required.
 * The latter can happen if an exception has already been raised in the frame.
 * The returned #GuExnData object's `data` member is initially `NULL`.
 */

#define gu_raise_new(exn, T, POOL, EXPR)				\
	GU_BEGIN							\
	GuExnData* gu_raise_err_ = gu_raise(exn, T);		\
	if (gu_raise_err_) {						\
		GuPool* POOL = gu_raise_err_->pool;			\
		gu_raise_err_->data = EXPR;				\
	}								\
	GU_END

#define gu_raise_i(error_, t_, ...) \
	gu_raise_new(error_, t_, gu_raise_pool_, gu_new_i(gu_raise_pool_, t_, __VA_ARGS__))


/** @name Checking for exceptions
 *
 * Because `libgu`'s exception framework does not automatically transfer the
 * control from the raiser to the handler, manual tests are needed to check
 * whether an exception has been raised by an operation. Typically, if an
 * exception has been raised and is not handled by the current function, the
 * function will return immediately, possibly after some local cleanup
 * operations.
 * 
 */

static inline bool
gu_exn_is_raised(GuExn* err);

/// Check the status of the current exception frame
static inline bool
gu_ok(GuExn* exn) {
	return !GU_UNLIKELY(gu_exn_is_raised(exn));
}
/**< @return `true` iff no exception has been raised in `exn` or propagated to
 * it from its subframes.
 */


/// Return from current function if an exception has been raised.
#define gu_return_on_exn(exn, retval)		\
	GU_BEGIN					\
	if (!gu_ok(exn)) return retval;	\
	GU_END
/**<
 * @showinitializer
 */


/** @name Handling exceptions
 * 
 * Once an exception frame has been passed to operations that may possibly
 * have raised exceptions on it. It can be checked with #gu_exn_caught to see
 * if that frame caught an exception that requires handling. If it did, then
 * #gu_exn_caught_data can be used to retrive the exception data.
 *
 * Typically, exception frames should correspond with stack frames, i.e. an
 * exception frame's handling should be done in same block where the frame was
 * created.
 */

/// The type of the exception caught by a frame
GuType*
gu_exn_caught(GuExn* exn);
/**<
 * @return The type of the exception caught by `exn`, or `NULL` if no
 * exception has been caught.
 *
 * @note The return type, if non-`NULL`, always belongs to the "catch kind" of
 * `exn`.
 */


/// The data of the exception caught by a frame
const void*
gu_exn_caught_data(GuExn* err);
/**<
 * @return A pointer to the data associated with the caught exception, or `NULL`.
 *
 * @note The returned pointer can be null even if an exception has been raised
 * (if no data is associated with the exception), so this function cannot be
 * used to reliably check if an exception was caught. Use #gu_exn_caught
 * instead.
 */



/** @name Special operations
 */

static inline void
gu_exn_clear(GuExn* err);

/// Temporarily block a raised exception.
void
gu_exn_block(GuExn* err);

/// Show again a blocked exception.
void
gu_exn_unblock(GuExn* err);







/// @name Utilities for errno

#include <errno.h>

typedef int GuErrno;

extern GU_DECLARE_TYPE(GuErrno, signed);



#define gu_raise_errno(error_) \
	gu_raise_i(error_, GuErrno, errno)

#if 0

typedef void (*GuExnPrintFn)(GuFn* clo, void* err, FILE* out);

extern GuTypeTable gu_exn_default_printer;

void
gu_exn_print(GuExn* err, FILE* out, GuTypeMap printer_map);

#endif

// Implementation bits

/// @private
typedef enum {
	GU_EXN_RAISED,
	GU_EXN_OK,
	GU_EXN_BLOCKED
} GuExnState;

/// @private
struct GuExn {
	/// @privatesection
	GuExnState state;
	GuExn* parent;
	GuKind* catch;
	GuType* caught;
	GuExnData data;
};

static inline void
gu_exn_clear(GuExn* err) {
	err->caught = NULL;
	err->state = GU_EXN_OK;
}

static inline bool
gu_exn_is_raised(GuExn* err) {
	return err && (err->state == GU_EXN_RAISED);
}



#endif // GU_EXN_H_
