#ifndef GU_MAP_H_
#define GU_MAP_H_

#include <gu/fun.h>
#include <gu/mem.h>

typedef const struct GuMapIterFn GuMapIterFn;

struct GuMapIterFn {
	void (*fn)(GuMapIterFn* self, const void* key, void* value);
};

typedef struct GuMap GuMap;

GuMap*
gu_map_new_full(GuPool* pool, GuHashFn* hash_fn, GuEqFn* eq_fn, 
		size_t key_size, size_t value_size, const void* empty_value);

static inline GuMap*
gu_map_new(GuPool* pool, GuHashFn* hash_fn, GuEqFn* eq_fn) {
	return gu_map_new_full(pool, hash_fn, eq_fn, 0, 0, NULL);
}

void*
gu_map_get(GuMap* ht, const void* key);

void
gu_map_set(GuMap* ht, const void* key, void* value);

void
gu_map_iter(GuMap* ht, GuMapIterFn* fn);

#define GU_MAP__SIZE_R(t_) (0)
#define GU_MAP__SIZE_V(t_) (sizeof(t_))

#define GU_MAP__KEYTYPE_R(t_) const t_*
#define GU_MAP__VALTYPE_R(t_) t_*
#define GU_MAP__KEYTYPE_V(t_) t_
#define GU_MAP__VALTYPE_V(t_) t_

#define GU_MAP__PASS_R(x_) (x_)
#define GU_MAP__PASS_V(x_) (&(x_))

#define GU_MAP__RET_R(t_, x_) (x_)
#define GU_MAP__RET_V(t_, x_) (*(t_*)(x_))

#define GU_MAP__EMPTY_R(t_, x_) (x_)
#define GU_MAP__EMPTY_V(t_, x_) (&(t_)(x_))

#define GU_MAP_DEFINE(t_, pfx, KM, k_, VM, v_, empty_v_, hash_, eq_) \
									\
typedef struct t_ t_;							\
									\
static inline t_* pfx##_new(GuPool* pool)				\
{									\
	GuMap* map = gu_map_new_full(pool, hash_, eq_,			\
				     GU_MAP__SIZE_##KM(k_),		\
				     GU_MAP__SIZE_##VM(v_),		\
				     GU_MAP__EMPTY_##VM(v_, empty_v_));	\
	return (t_*) map;						\
}									\
									\
static inline void							\
pfx##_set(t_* map,							\
	  GU_MAP__KEYTYPE_##KM(k_) key, GU_MAP__VALTYPE_##VM(v_) val)	\
{									\
	gu_map_set((GuMap*) map,					\
		   GU_MAP__PASS_##KM(key), GU_MAP__PASS_##VM(val));	\
}									\
									\
static inline GU_MAP__VALTYPE_##VM(v_)					\
pfx##_get(t_* map, GU_MAP__KEYTYPE_##KM(k_) key)			\
{									\
	void* val = gu_map_get((GuMap*) map, GU_MAP__PASS_##KM(key));	\
	return GU_MAP__RET_##VM(v_, val);				\
}									\
GU_DECLARE_DUMMY


				    
#include <gu/string.h>

GU_MAP_DEFINE(GuStringMap, gu_stringmap, R, GuString, R, void, NULL,
	      &gu_string_hash, &gu_string_eq);
GU_MAP_DEFINE(GuIntMap, gu_intmap, V, int, R, void, NULL,
	      &gu_int_hash, &gu_int_eq);


#include <gu/type.h>

extern GU_DECLARE_KIND(GuMap);

typedef const struct GuMapType GuMapType, GuType_GuMap;

struct GuMapType {
	GuType_abstract abstract_base;
	GuHashFn* hash_fn;
	GuEqFn* eq_fn;
	bool direct_key;
	bool direct_value;
	GuType* key_type;
	GuType* value_type;
	const void* empty_value;
};

GuMap*
gu_map_type_make(GuMapType* mtype, GuPool* pool);

#define GU_TYPE_INIT_GuMap(k_, t_, h_, eq_, dk_, kt_, dv_, vt_, ev_) {	\
	.abstract_base = GU_TYPE_INIT_abstract(k_, t_, _),		\
	.hash_fn = h_,					\
	.eq_fn = eq_,					\
	.direct_key = dk_,			\
	.direct_value = dv_,			\
	.key_type = kt_,			\
	.value_type = vt_,		\
	.empty_value = ev_ \
}

#define GuPtrMap GuMap
#define GU_TYPE_INIT_GuPtrMap(k_, t_, h_, eq_, kt_, vt_)	\
	GU_TYPE_INIT_GuMap(k_, t_, h_, eq_, false, kt_, false, vt_, NULL)

extern GU_DECLARE_KIND(GuIntMap);
typedef GuType_GuMap GuType_GuIntMap;
#define GU_TYPE_INIT_GuIntMap(k_, t_, dv_, vt_, ev_)		\
	GU_TYPE_INIT_GuMap(k_, t_, &gu_int_hash, &gu_int_eq,	\
			   true, gu_type(int), dv_, vt_, ev_)

#define GuIntPtrMap GuIntMap
#define GU_TYPE_INIT_GuIntPtrMap(k_, t_, vt_)	\
	GU_TYPE_INIT_GuIntMap(k_, t_, false, vt_, NULL)

extern GU_DECLARE_KIND(GuStringMap);
typedef GuType_GuMap GuType_GuStringMap;
#define GU_TYPE_INIT_GuStringMap(k_, t_, dv_, vt_, ev_)			\
	GU_TYPE_INIT_GuMap(k_, t_, &gu_string_hash, &gu_string_eq,	\
			   false, gu_type(GuString), dv_, vt_, ev_)

#define GuStringPtrMap GuStringMap
#define GU_TYPE_INIT_GuStringPtrMap(k_, t_, vt_)	\
	GU_TYPE_INIT_GuStringMap(k_, t_, false, vt_, NULL)



#endif // GU_MAP_H_
