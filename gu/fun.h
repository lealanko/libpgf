// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#ifndef GU_FUN_H_
#define GU_FUN_H_

#include <gu/defs.h>

typedef void (*GuFn)();
typedef void (*GuFn0)(GuFn* clo);
typedef void (*GuFn1)(GuFn* clo, void* arg1);
typedef void (*GuFn2)(GuFn* clo, void* arg1, void* arg2);

#define gu_fn(fn_) (&(GuFn){ fn_ })

static inline void
gu_apply0(GuFn* fn) {
	(*fn)(fn);
}

static inline void
gu_apply1(GuFn* fn, void* arg1) {
	(*fn)(fn, arg1);
}

static inline void
gu_apply2(GuFn* fn, void* arg1, void* arg2) {
	(*fn)(fn, arg1, arg2);
}

#define gu_apply(fn_, ...)			\
	((fn_)->fn((fn_), __VA_ARGS__))

typedef struct GuClo0 GuClo0;

struct GuClo0 {
	GuFn fn;
};

typedef struct GuClo1 GuClo1;

struct GuClo1 {
	GuFn fn;
	void *env1;
};

typedef struct GuClo2 GuClo2;
struct GuClo2 {
	GuFn fn;
	void *env1;
	void *env2;
};

typedef struct GuClo3 GuClo3;
struct GuClo3 {
	GuFn fn;
	void *env1;
	void *env2;
	void *env3;
};



#ifdef GU_GNUC
#define gu_invoke(OBJECT, METHOD, ...)			  \
	({						  \
		typeof(OBJECT) obj_ = (OBJECT);		  \
		obj_->funs->METHOD(obj_, __VA_ARGS__);	  \
	})
#else
#define gu_invoke(OBJECT, METHOD, ...)			  \
	((OBJECT)->funs->METHOD((OBJECT), __VA_ARGS))
#endif

typedef const struct GuEq GuEq;
typedef struct GuEqFuns GuEqFuns;

struct GuEqFuns {
	bool (*is_equal)(GuEq* self, const void* a, const void* b);
};

struct GuEq {
	GuEqFuns* funs;
};

static inline bool
gu_eq(GuEq* eq, const void* a, const void* b)
{
	return (a == b) || gu_invoke(eq, is_equal, a, b);
}

#endif // GU_FUN_H_
