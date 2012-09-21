// Copyright 2010-2012 University of Helsinki. Released under LGPL3.

/** @file
 *
 * Memory management operations.
 *
 * Memory management in `libgu` is based on memory pools, also known as
 * "regions", "arenas" or "zones". Memory pools can be created and destroyed
 * individually, but other objects are always allocated from a pool, and
 * remain alive until the pool gets destroyed. It is up to the programmer to
 * ensure that a pool isn't destroyed while some of its objects are still in
 * use.
 *
 * When a function needs to return dynamically allocated objects, the caller
 * must provide a memory pool from which the objects are to be allocated.
 * Hence, a typical function would look roughly like this:
 *
 *     Foo* create_something(Bar* bar, GuPool* pool)
 *     {
 *             GuPool* tmp_pool = gu_new_pool(); // for locally used objects
 *             Foo* foo = gu_new(pool, Foo);
 *
 *             // do local calculations using `tmp_pool`
 *             // allocate things to be stored in `foo` using `pool`
 *
 *             gu_pool_free(tmp_pool); // release all temporary storage
 *             return foo;
 *     }        
 * 
 * 
 */

#ifndef GU_MEM_H_
#define GU_MEM_H_

#include <gu/defs.h>
#include <gu/fun.h>


/// @private
// this has to be outside sections, otherwise @private won't work
#define GU_LOCAL_POOL_INIT_SIZE_ (16 * sizeof(GuWord))

/** @name Creating a pool
 *
 * There are two ways to create a memory pool. The #gu_new_pool function
 * should be used in most situations. In performance-critical sections the
 * #gu_local_pool macro can be used instead, at the cost of some stack usage.
 * 
 */

/// A memory pool.
typedef struct GuPool GuPool;

/// Create a new memory pool.
GU_ONLY GuPool*
gu_new_pool(void);

/**< 
 * @return A new memory pool.
 *
 * @note The returned pool must be freed with #gu_pool_free
 * once the objects allocated from it are no longer used.
 */


/// @private
GuPool*
gu_local_pool_(GuSlice init_buf);

/// Create a stack-allocated memory pool.
#define gu_local_pool()				\
	gu_local_pool_(gu_slice(gu_alloca(GU_LOCAL_POOL_INIT_SIZE_),	\
				GU_LOCAL_POOL_INIT_SIZE_))
/**<
 * @return A memory pool whose first chunk is allocated directly from
 * the stack. This makes its creation faster, and more suitable for
 * functions that usually allocate only a little memory from the pool
 * until it is freed.
 *
 * @note The pool created with #gu_local_pool \e must be freed with
 * #gu_pool_free before the end of the block where #gu_local_pool was
 * called.
 *
 * @note Because #gu_local_pool uses relatively much stack space, it
 * should not be used in the bodies of recursive functions.
 */


/** @name Allocating from a pool
 * 
 * Once a memory pool has been created, objects can be allocated from it.
 * These objects remain alive until the pool is destroyed.
 * 
 * The most convenient way to allocate objects of a fixed type is to use the
 * #gu_new macro. If the type of the object can vary, the #gu_malloc_aligned
 * function can be used to allocate an object with a size and alignment that
 * are determined at run-time.
 *
 * If memory allocation fails, the allocation functions will abort the
 * execution of the program. Hence, if a function returns, it is guaranteed to
 * return a pointer to a valid object.
 */
 

/// Allocate an object with a given size and alignment.
void* 
gu_malloc_aligned(GuPool* pool, size_t size, size_t alignment);

/**< @return A pointer to space for an object of size `size` and alignment of
 * at least `alignment`, allocated from `pool`.
 */


void*
gu_malloc_prefixed(GuPool* pool, size_t pre_align, size_t pre_size,
		   size_t align, size_t size);


/// Allocate an object with a given size.
inline void*
gu_malloc(GuPool* pool, size_t size) {
	return gu_malloc_aligned(pool, size, 0);
}
/**< @return A pointer to space for an object of size `size` and any
 * alignment, allocated from `pool`.
 */

GuSlice
gu_malloc_slice(GuPool* pool, size_t size);

#include <string.h>

//@private
static inline void*
gu_malloc_init_aligned(GuPool* pool, size_t size, size_t alignment, 
		       const void* init)
{
	void* p = gu_malloc_aligned(pool, size, alignment);
	memcpy(p, init, size);
	return p;
}

//@private
static inline void*
gu_malloc_init(GuPool* pool, size_t size, const void* init)
{
	return gu_malloc_init_aligned(pool, size, 0, init);
}


/// Allocate space to store an array of objects of a given type.

#define gu_new_n(TYPE, n, pool)						\
	((TYPE*)gu_malloc_aligned((pool),				\
				  sizeof(TYPE) * (n),			\
				  gu_alignof(TYPE)))
/**< 
 * @param TYPE  type
 *
 * @param n     `size_t`
 * 
 * @param pool  #GuPool*
 *
 * @return A pointer to space for an array of `n` objects of type `TYPE`,
 * allocated from `pool`.
 */ 


/// Allocate space to store an object of a given type.

#define gu_new(TYPE, pool) \
	gu_new_n(TYPE, 1, pool)
/**< 
 * @param TYPE  type
 *
 * @param pool  #GuPool*
 *
 * @return A pointer to space for an object of type `TYPE`, allocated from
 * `pool`.
 */


#define gu_new_prefixed(pre_type, type, pool)				\
	((type*)(gu_malloc_prefixed((pool),				\
				    gu_alignof(pre_type), sizeof(pre_type), \
				    gu_alignof(type), sizeof(type))))



#ifdef GU_HAVE_STATEMENT_EXPRESSIONS
#define gu_new_i(pool, type, ...)					\
	({								\
		type *gu_new_p_ = gu_new(type, pool);			\
		memcpy((void*) gu_new_p_, &(type){ __VA_ARGS__ },	\
		       sizeof(type));					\
		gu_new_p_;						\
	})
#else // GU_HAVE_STATEMENT_EXPRESSIONS
#define gu_new_i(pool, TYPE, ...)					\
	((type*)gu_malloc_init_aligned((pool), sizeof(TYPE),		\
				       gu_alignof(type),		\
				       &(type){ __VA_ARGS__ }))
#endif // GU_HAVE_STATEMENT_EXPRESSIONS

/** @def gu_new_i(pool, TYPE, ...)
 *
 * Allocate and initialize an object. This 
 *
 * @param pool #GuPool*
 *
 * @param TYPE type
 *
 * @param ...  initializer list
 *
 * @return A pointer to an object of type `TYPE` allocated from `pool`,
 * initialized with the given initializer list.
 *
 * @example 
 *
 *     int* ip = gu_new_i(pool, int, 42);
 *
 * @example
 *
 *     typedef struct { int x; int y; } Point;
 *
 *     ...
 *
 *     Point* p = gu_new(pool, Point, .x = 3, .y = 4);
 */

#define gu_new_s gu_new_i

// Alas, there's no portable way to get the alignment of flex structs.
#define gu_new_flex(pool_, type_, flex_member_, n_elems_)		\
	((type_ *)gu_malloc_aligned(					\
		(pool_),						\
		GU_FLEX_SIZE(type_, flex_member_, n_elems_),		\
		gu_flex_alignof(type_)))


/** @name Finalizers
 *
 * Some data structures may use other resources besides objects allocated from
 * a memory pool. Finalizers provide a way to free these resources when a pool
 * is freed.
 *
 */


/// A finalizer.
typedef struct GuFinalizer GuFinalizer;



/// A finalizer.
struct GuFinalizer {
	/// Run finalization
	void (*fn)(GuFinalizer* self);
	/**< This function will perform finalization task. The parameter `self` will
	 * always be a pointer to the finalizer object itself. */
};

/// Register a finalizer.
void
gu_pool_finally(GuPool* pool, GuFinalizer* fini);

/**< The finalizer `fini` will be called when `pool` is destroyed. The
 * finalizers are called in reverse order of registration.
 */


/** @name Destroying a pool
 *
 * Once a memory pool and the objects allocated from it are no longer used, it
 * must be freed with #gu_pool_free.
 */

/// Free a memory pool and all objects allocated from it.
void
gu_pool_free(GU_ONLY GuPool* pool);
/**<
 * When the pool is freed, all finalizers registered by
 * #gu_pool_finally on `pool` are invoked in reverse order of
 * registration.
 * 
 * @note After the pool is freed, all objects allocated from it become
 * invalid and may no longer be used. */





/** @name Memory buffers
 *
 * Memory allocated from pools cannot be resized or freed before the pool
 * itself is destroyed. Some data structures, however, require contiguous
 * blocks of memory that need to grow on demand. For these, memory buffers can
 * be used. Memory buffers are simply manually managed, resizable blocks of
 * heap-allocated memory. A typical data structure will allocate its
 * fixed-size parts from a memory pool, and its dynamically sized parts as
 * memory buffers, and register a finalizer to free the memory buffers when
 * the pool is destroyed.
 * 
 * These operations differ from standard `malloc`, `realloc` and `free`
 * -functions in that memory buffers are not allocated by exact size. Instead,
 * a minimum size is requested, and the returned buffer may be larger. This
 * gives the memory allocator more flexibility to optimize memory usage when
 * the client code can make use of larger buffers than requested.
 */

/// Allocate a new memory buffer.
GuSlice
gu_mem_buf_alloc(size_t min_size);
/**<
 * @param min_size
 * The minimum acceptable size for a returned memory block.
 *
 * @param[out] real_size
 * The actual size of the returned memory block. This is never less than
 * `min_size`.
 * 
 * @return A pointer to a block of memory at least `min_size` bytes long and
 * suitably aligned for storing any object.
 *
 * @note The returned pointer must be freed with #gu_mem_buf_free or
 * reallocated with #gu_mem_buf_realloc.
 */


/// Resize or reallocate a memory buffer
GuSlice
gu_mem_buf_realloc(
	GU_NULL GU_ONLY GU_RETURNED
	void* buf,
	size_t min_size);
/**<
 *
 * @param buf
 * A pointer that was previously returned by #gu_mem_buf_alloc or
 * #gu_mem_buf_realloc, or `NULL`. If non-`NULL`, the pointer can no longer be
 * used once this function returns.
 * 
 * @param min_size
 * The minimum acceptable size for a returned memory block.
 *
 * @param[out] real_size_out
 * The actual size of the returned memory block. This is never less than
 * `min_size`.
 * 
 * @return A pointer to a block of memory at least `min_size` bytes long and
 * suitably aligned for storing any object. The returned pointer may or may
 * not be equal to `buf`.
 *
 * @note The returned pointer must be freed with #gu_mem_buf_free or
 * reallocated with #gu_mem_buf_realloc.
 */


/// Free a memory buffer.
void
gu_mem_buf_free(GU_ONLY void* buf);
/**<
 *
 * @param buf
 * A pointer that was previously returned by #gu_mem_buf_alloc or
 * #gu_mem_buf_realloc. The pointed memory is freed.
 */





#endif // GU_MEM_H_
