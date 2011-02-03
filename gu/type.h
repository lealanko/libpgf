 
#ifndef GU_TYPE_H_
#define GU_TYPE_H_

#include <gu/defs.h>

//
// kind
//

typedef const struct GuKind GuKind;

struct GuKind {
	GuKind* super;
};

#define gu_kind(k_) ((GuKind*)&gu_type_##k_)

#define GU_DECLARE_KIND(k_) \
	GuKind gu_type_##k_

extern GU_DECLARE_KIND(kind);

#define GU_DEFINE_KIND(k_, super_k_) \
	GuKind gu_type_##k_ = { .super = gu_kind(super_k_) }

//
// type
//

typedef const struct GuType GuType;

struct GuType {
	GuKind kind_base;
};

typedef GuType GuType_type;

extern GU_DECLARE_KIND(type);

#define GU_TYPE_INIT_type(k_, t_, _) { .kind_base = { .super = gu_kind(k_) } }

#define gu_type(t_) ((GuType*)gu_kind(t_))


#define GU_TYPE_INIT(k_, ...)		\
	GU_TYPE_INIT_##k_(k_, __VA_ARGS__)

#define GU_TYPE_LIT(k_, ...)				\
	((GuType*)(GuType_##k_[]){GU_TYPE_INIT(k_, __VA_ARGS__)})

#define GU_DECLARE_TYPE(t_, k_)	\
	GuType_##k_ gu_type_##t_

#define GU_DEFINE_TYPE(t_, k_, ...)					\
	GuType_##k_ gu_type_##t_ = GU_TYPE_INIT(k_, t_, __VA_ARGS__)


//
// abstract
//

typedef GuType GuType_abstract;

#define GU_TYPE_INIT_abstract(k_, t_, _)		\
	GU_TYPE_INIT_type(k_, t_, _)

extern GU_DECLARE_KIND(abstract);



//
// typedef
//


// We must include string.h only after abstract has been defined.

#include <gu/string.h>

typedef const struct GuTypeDef GuTypeDef;
typedef GuTypeDef GuType_typedef;

struct GuTypeDef {
	GuType type_base;
	const GuString* name;
	GuType* type;
};

#define GU_TYPE_INIT_typedef(k_, t_, type_) {			\
		.type_base = GU_TYPE_INIT_type(k_, t_, _),	\
		.name = gu_cstring(#t_),		\
		.type = type_				\
}

extern GU_DECLARE_KIND(typedef);

#define GU_DEFINE_TYPEDEF_X(t_, dk_, ...)	\
	GU_DEFINE_TYPE(t_, dk_, t_, GU_TYPE_LIT(__VA_ARGS__))

#define GU_DEFINE_TYPEDEF(t_, ...)	\
	GU_DEFINE_TYPEDEF_X(t_, typedef, __VA_ARGS__)



//
// referenced
//

extern GU_DECLARE_KIND(referenced);

typedef GuType_typedef GuType_referenced;

#define GU_TYPE_INIT_referenced GU_TYPE_INIT_aliased


//
// repr 
//

typedef struct GuTypeRepr GuType_repr;

struct GuTypeRepr {
	GuType type_base;
	size_t size;
	size_t align;
};

#define GU_TYPE_INIT_repr(k_, t_, _) {				\
		.type_base = GU_TYPE_INIT_type(k_, t_, _),	\
		.size = sizeof(t_), \
		.align = gu_alignof(t_) \
		 }

extern GU_DECLARE_KIND(repr);



#include <gu/list.h>

//
// struct
//

typedef const struct GuStructRepr GuStructRepr;
typedef GuStructRepr GuType_struct;

typedef const struct GuMember GuMember;

struct GuMember {
	ptrdiff_t offset;
	const GuString* name;
	GuType* type;
	bool is_flex;
};

struct GuStructRepr {
	GuType_repr repr_base;
	GuSList(GuMember) members;
};

extern GU_DECLARE_KIND(struct);

#define GU_MEMBER_AUX_(struct_, member_, type_, is_flex_)      \
	{					      \
		.offset = offsetof(struct_, member_), \
		.name = gu_cstring(#member_),	      \
		.type = type_,		      \
		.is_flex = is_flex_,  \
	}

#define GU_MEMBER_V(struct_, member_, type_) \
	GU_MEMBER_AUX_(struct_, member_, type_, false)

#define GU_MEMBER(s_, m_, t_) \
	GU_MEMBER_V(s_, m_, gu_type(t_))

#define GU_FLEX_MEMBER_V(struct_, member_, type_) \
	GU_MEMBER_AUX_(struct_, member_, type_, true)

#define GU_FLEX_MEMBER(s_, m_, t_) \
	GU_FLEX_MEMBER_V(s_, m_, gu_type(t_))


#define GU_TYPE_INIT_struct(k_, t_, ...)	{		\
		.repr_base = GU_TYPE_INIT_repr(k_, t_, _),	\
	.members = GU_SLIST(GuMember, __VA_ARGS__)	\
}



//
// pointer
//

typedef const struct GuPointerType GuPointerType;
typedef GuPointerType GuType_pointer;

struct GuPointerType {
	GuType_repr repr_base;
	GuType* pointed_type;
};

#define GU_TYPE_INIT_pointer(k_, t_, pointed_)	\
	{					   \
		.repr_base = GU_TYPE_INIT_repr(k_, t_, _),	\
	.pointed_type = pointed_ \
}	


extern GU_DECLARE_KIND(pointer);


//
// reference
//

typedef GuType_pointer GuType_reference;

#define GU_TYPE_INIT_reference GU_TYPE_INIT_pointer

extern GU_DECLARE_KIND(reference);


//
// shared
//

typedef GuType_pointer GuType_shared;

#define GU_TYPE_INIT_shared GU_TYPE_INIT_pointer

extern GU_DECLARE_KIND(shared);


//
// primitives
//

typedef const struct GuPrimType GuType_primitive;

struct GuPrimType {
	GuType_repr repr_base;
	const GuString* name;
};

#define GU_TYPE_INIT_primitive(k_, t_, _) {	\
	.repr_base = GU_TYPE_INIT_repr(k_, t_, _),	\
	.name = gu_cstring(#t_)			\
}	

extern GU_DECLARE_KIND(primitive);


typedef GuType_primitive GuType_integer;
#define GU_TYPE_INIT_integer GU_TYPE_INIT_primitive
extern GU_DECLARE_KIND(integer);

typedef GuType_integer GuType_int;
#define GU_TYPE_INIT_int GU_TYPE_INIT_integer
extern GU_DECLARE_TYPE(int, primitive);

typedef int GuLength;
extern GU_DECLARE_TYPE(GuLength, int);


//
// enum
//

extern GU_DECLARE_KIND(enum);

typedef const struct GuEnumConstant GuEnumConstant;

struct GuEnumConstant {
	const GuString* name;
	long long value;
};

typedef const struct GuEnumType GuType_enum;

struct GuEnumType {
	GuType_repr repr_base;
	GuSList(GuEnumConstant) constants;
};

#define GU_ENUM_C(x) { .name = gu_cstring(#x), .value = x }

#define GU_TYPE_INIT_enum(k_, t_, ...) { \
	.repr_base = GU_TYPE_INIT_repr(k_, t_, _),	\
	.constants = GU_SLIST(GuEnumConstant, __VA_ARGS__) \
}








bool gu_type_has_kind(const GuType* type, const GuKind* kind);




typedef const struct GuTypeTableEntry GuTypeTableEntry;

struct GuTypeTableEntry {
	GuKind* kind;
	void* val;
};

typedef const struct GuTypeTable GuTypeTable;

struct GuTypeTable {
	GuSList(const GuTypeTable*) parents;
	GuSList(GuTypeTableEntry) entries;
};

#define GU_TYPETABLE(parents_, ...) { \
	.parents = parents_, \
	.entries = GU_SLIST(GuTypeTableEntry, \
			    __VA_ARGS__) \
 }



#include <gu/map.h>

typedef GuMap GuTypeMap;

GuTypeMap* gu_type_map_new(GuPool* pool, GuTypeTable* table);

void* gu_type_map_lookup(GuTypeMap* tmap, GuType* type);

size_t gu_type_size(GuType* type);

const void*
gu_type_check_cast(GuType* t, GuKind* k);

#define gu_type_cast(type_, k_) \
	((GuType_##k_*)gu_type_check_cast(type_, gu_kind(k_)))




#endif // GU_TYPE_H_
