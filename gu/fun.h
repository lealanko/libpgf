#ifndef GU_FUN_H_
#define GU_FUN_H_

typedef void (*GuFn)();
typedef void (*GuFn0)(GuFn* clo);
typedef void (*GuFn1)(GuFn* clo, void* arg1);
typedef void (*GuFn2)(GuFn* clo, void* arg1, void* arg2);

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

#define gu_fn(fn) ((GuFn[1]){ fn })

#endif // GU_FUN_H_
