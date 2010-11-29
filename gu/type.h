
#ifndef GU_TYPE_H_
#define GU_TYPE_H_

#include <gu/defs.h>
#include <gu/flex.h>
#include <gu/map.h>

typedef const struct GuKind GuKind;

struct GuKind {
	GuKind* super;
}; 


typedef const struct GuType GuType;

struct GuType {
	const GuKind kind; // singleton kind
	size_t align;
	size_t size;
};



#define GU_KIND_P(t_) \
	((GuKind*)&gu_type_##t_)

#define GU_TYPE_P(t_) \
	((GuType*)&gu_type_##t_)

#define GU_TYPE_INIT(t_, super_, ...)				\
	GU_TYPE_##super_##_C(t_, gu_type_##super_, __VA_ARGS__)

#define GU_TYPE_NEW(t_, super_, ...)				\
	((GuType*)(GuType_##super_[]){GU_TYPE_INIT(t_, super_, __VA_ARGS__)})


#define GU_DECLARE_TYPE(t_, super_)	\
	GuType_##super_ gu_type_##t_

#define GU_DEFINE_TYPE(t_, super_, ...)					\
	GuType_##super_ gu_type_##t_ = GU_TYPE_INIT(t_, super_, __VA_ARGS__)

#define GU_TYPE__top_C(t_, super_) {			\
	.kind = { .super = (GuKind*)(&super_) },	\
	.align = gu_alignof(t_),	\
	.size = sizeof(t_)		\
}

extern GuKind gu_type__top;
typedef GuType GuType__top;



typedef const struct GuNamedType GuType__named;

struct GuNamedType {
	GuType__top top;
	const char* name;
};

extern GuKind gu_type__named;

#define GU_TYPE__named_C(t_, super_, ...) {				\
	.top = GU_TYPE__top_C(t_, super_),			\
	.name = #t_				\
}




typedef const struct GuMember GuMember;

struct GuMember {
	ptrdiff_t offset;
	const char* name;
	GuType* type;
};

#define GU_MEMBER_V(struct_, member_, type_)	      \
	{					      \
		.offset = offsetof(struct_, member_), \
		.name = #member_,		      \
		.type = type_		      \
	}

#define GU_MEMBER(s_, m_, t_) \
	GU_MEMBER_V(s_, m_, GU_TYPE_P(t_))


typedef const struct GuStructType GuType__struct;

struct GuStructType {
	GuType__named named;
	GuSList(GuMember) members;
};

#define GU_TYPE__struct_C(t_, super_, ...)	{	\
	.named = GU_TYPE__named_C(t_, super_),		\
	.members = GU_SLIST(GuMember, __VA_ARGS__)	\
}

extern GuKind gu_type__struct;



typedef const struct GuPointerType GuType__pointer;

struct GuPointerType {
	GuType__top top;
	GuType* pointed_type;
};

#define GU_TYPE__pointer_C(t_, super_, pointed_)	\
	{ \
	.top = GU_TYPE__top_C(t_, super_), \
	.pointed_type = pointed_ \
}	




typedef GuType__named GuType_int;

extern GuType_int gu_type_int;




typedef GuType__struct GuType_GuList;

#define GU_TYPE_GuList_C(super_, t_, elem_t_) {			\
	GU_TYPE_struct_C(super_, t_,					\
		GU_MEMBER(t_, len, &gu_type_int),			\
		GU_MEMBER(t_, elems, elem_t_))





typedef const struct GuVariantType GuType_GuVariant;

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
	GU_CONSTRUCTOR_V(ctag, GU_TYPE_P(t_))

#define GU_CONSTRUCTOR_S(ctag, t_, ...)		\
	GU_CONSTRUCTOR_V(ctag, GU_TYPE_NEW(t_, _struct, __VA_ARGS__))


#define GU_TYPE_GuVariant_C(t_, super_, ...) {	\
	.named = GU_TYPE__named_C(t_, super_), \
	.ctors = GU_SLIST(GuConstructor, __VA_ARGS__) \
}


typedef GuSList(GuConstructor) GuConstructors;

struct GuVariantType {
	GuType__named named;
	GuConstructors ctors;
};

extern GuKind gu_type_GuVariant;







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
