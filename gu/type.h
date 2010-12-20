
#ifndef GU_TYPE_H_
#define GU_TYPE_H_

#include <gu/defs.h>
#include <gu/flex.h>
#include <gu/map.h>


//
// kind
//

typedef const struct GuKind GuKind;

struct GuKind {
	GuKind* super;
};

#define GU_TYPE_INIT_kind(t_, k_, super_) { .super = super_ }

#define GU_KIND(k_) ((GuKind*)&gu_type_##k_)

#define GU_DECLARE_KIND(k_) \
	GuKind gu_type_##k_

extern GU_DECLARE_KIND(kind);

#define GU_DEFINE_KIND(k_, super_) \
	GuKind gu_type_##k_ = { .super = super_ }

//
// type
//

typedef const struct GuType GuType;

struct GuType {
	GuKind kind_base;
};

typedef GuType GuType_type;

extern GU_DECLARE_KIND(type);

#define GU_TYPE_INIT_type(t_, k_) { .kind_base = GU_KIND(k_) }

#define GU_TYPE(t_) ((GuType*)GU_KIND(t_))


#define GU_TYPE_INIT(t_, k_, ...)		\
	GU_TYPE_INIT_##k_(t_, k_, __VA_ARGS__)

#define GU_TYPE_LIT(t_, k_, ...)				\
	((GuType*)(GuType_##k_[]){GU_TYPE_INIT(t_, k_, __VA_ARGS__)})

#define GU_DECLARE_TYPE(t_, k_)	\
	GuType_##k_ gu_type_##t_

#define GU_DEFINE_TYPE(t_, k_, ...)					\
	GuType_##k_ gu_type_##t_ = GU_TYPE_INIT(t_, k_, __VA_ARGS__)


//
// typedef
//

typedef struct GuTypeDef GuType_typedef;

struct GuTypeDef {
	GuType type_base;
	const char* name;
	GuType* type;
};

#define GU_TYPE_INIT_typedef(t_, k_, type_) {		\
		.type_base = GU_TYPE_INIT_type(t_, k_),	\
			.name = #t_,			\
			.type = type_			\
}

extern GU_DECLARE_KIND(typedef);

#define GU_DEFINE_TYPEDEF_X(t_, dk_, ...)	\
	GU_DEFINE_TYPE(t_, dk_, GU_TYPE_LIT(t_, __VA_ARGS__))

#define GU_DEFINE_TYPEDEF(t_, ...)	\
	GU_DEFINE_TYPEDEF_X(t_, typedef, __VA_ARGS__)


//
// repr 
//

typedef struct GuTypeRepr GuType_repr;

struct GuTypeRepr {
	GuType type_base;
	size_t size;
	size_t align;
};

// Third parameter needed because empty __VA_ARGS__ produces one
// dummy argument
#define GU_TYPE_INIT_repr(t_, k_, _) {		     \
	.type_base = GU_TYPE_INIT_type(t_, k_),      \
		.size = sizeof(t_), \
		.align = gu_alignof(t_) \
		 }

extern GU_DECLARE_KIND(repr);


//
// struct
//



typedef const struct GuStructRepr GuType_struct;

typedef const struct GuMember GuMember;

struct GuMember {
	ptrdiff_t offset;
	const char* name;
	GuType* type;
};

struct GuStructRepr {
	GuType_repr repr_base;
	GuType_struct* super;
	GuSList(GuMember) members;
};

extern GU_DECLARE_KIND(struct);

#define GU_MEMBER_V(struct_, member_, type_)	      \
	{					      \
		.offset = offsetof(struct_, member_), \
		.name = #member_,		      \
		.type = type_		      \
	}

#define GU_MEMBER(s_, m_, t_) \
	GU_MEMBER_V(s_, m_, GU_TYPE(t_))

#define GU_TYPE_INIT_struct(t_, kind_, ...)	{	\
		.repr_base = GU_TYPE_INIT_repr(t_, kind_, _),	\
	.members = GU_SLIST(GuMember, __VA_ARGS__)	\
}



//
// pointer
//

typedef const struct GuPointerType GuType_pointer;

struct GuPointerType {
	GuType_repr repr_base;
	GuType* pointed_type;
};

#define GU_TYPE_INIT_pointer(t_, kind_, pointed_) \
	{					   \
	.repr_base = GU_TYPE_INIT_repr(t_, kind_, _),	\
	.pointed_type = pointed_ \
}	

extern GU_DECLARE_KIND(pointed);



//
// primitives
//

typedef const struct GuPrimType GuType_primitive;

struct GuPrimType {
	GuType_repr repr_base;
	const char* name;
};

#define GU_TYPE_INIT_primitive(t_, k_, _) { \
	.repr_base = GU_TYPE_INIT_repr(t_, k_, _),	\
	.name = #t_ \
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
// list
//

typedef GuType_struct GuType_GuList;

#define GU_TYPE_INIT_GuList(t_, kind_, elem_type_) \
	GU_TYPE_INIT_struct(t_, kind_, \
		GU_MEMBER(t_, len, GU_TYPE(GuLength)), \
	        GU_MEMBER(t_, elems, elem_type_))

extern GU_DECLARE_KIND(GuList);



// variant


typedef const struct GuConstructor GuConstructor;

struct GuConstructor {
	int c_tag;
	const GuType* type;
};

#define GU_CONSTRUCTOR_V(ctag, c_type) {		\
		.c_tag = ctag,	 \
		.type = c_type \
}

#define GU_CONSTRUCTOR(ctag, t_) \
	GU_CONSTRUCTOR_V(ctag, GU_TYPE(t_))

typedef GuSList(GuConstructor) GuConstructors;


typedef const struct GuVariantType GuType_GuVariant;

struct GuVariantType {
	GuType_repr repr_base;
	GuConstructors ctors;
};

#define GU_TYPE_INIT_GuVariant(t_, k_, ...) {	\
	.repr_base = GU_TYPE_INIT_repr(t_, k_, _),  \
	.ctors = GU_SLIST(GuConstructor, __VA_ARGS__) \
}

extern GU_DECLARE_KIND(GuVariant);



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




typedef GuMap GuTypeMap;

GuTypeMap* gu_type_map_new(GuPool* pool, GuTypeTable* table);

void* gu_type_map_lookup(GuTypeMap* tmap, GuType* type);

#endif // GU_TYPE_H_
