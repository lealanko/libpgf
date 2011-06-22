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
		size_t key_size, size_t value_size, void* empty_value);

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

#if 0

typedef GuMap GuStringMap;

static inline GuStringMap*
gu_stringmap_new(GuPool* pool)
{
	return gu_map_new(pool, &gu_string_hash, &gu_string_eq);
}

static inline void*
gu_stringmap_get(GuStringMap* m, const GuString* key)
{
	return gu_map_get(m, key);
}

static inline void
gu_stringmap_set(GuStringMap* m, const GuString* key, void* value)
{
	gu_map_set(m, key, value);
}
#else
GU_MAP_DEFINE(GuStringMap, gu_stringmap, R, GuString, R, void, NULL,
	      &gu_string_hash, &gu_string_eq);

#endif

typedef GuMap GuIntMap;

static inline GuIntMap*
gu_intmap_new(GuPool* pool)
{
	return gu_map_new_full(pool, &gu_int_hash, &gu_int_eq, 
			       sizeof(int), 0, NULL);
}

static inline void*
gu_intmap_get(GuIntMap* m, int key)
{
	return gu_map_get(m, &key);
}

static inline void
gu_intmap_set(GuIntMap* m, int key, void* value)
{
	gu_map_set(m, &key, value);
}


#include <gu/type.h>

extern GU_DECLARE_KIND(GuMap);

typedef const struct GuMapType GuMapType, GuType_GuMap;

struct GuMapType {
	GuType_abstract abstract_base;
	GuType* key_type;
	GuType* value_type;
};

#define GU_TYPE_INIT_GuMap(k_, t_, key_type_, value_type_) {	\
	.abstract_base = GU_TYPE_INIT_abstract(k_, t_, _),	\
	.key_type = key_type_, \
	.value_type = value_type_ \
}	

extern GU_DECLARE_KIND(GuIntMap);
typedef GuType_GuMap GuType_GuIntMap;

#define GU_TYPE_INIT_GuIntMap(k_, t_, value_type_) \
	GU_TYPE_INIT_GuMap(k_, t_, gu_type(int), value_type_)

#include <gu/string.h>

extern GU_DECLARE_KIND(GuStringMap);
typedef GuType_GuMap GuType_GuStringMap;

#define GU_TYPE_INIT_GuStringMap(k_, t_, value_type_) \
	GU_TYPE_INIT_GuMap(k_, t_, gu_type(GuString), value_type_)






#endif // GU_MAP_H_
