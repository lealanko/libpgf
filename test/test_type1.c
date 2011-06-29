
#include <stdio.h>
#include <gu/type.h>
#include <gu/variant.h>
#include <gu/mem.h>

typedef struct {
	int x;
	int y;
} Point;

#if 1
GU_DEFINE_TYPE(Point, struct, 
	       GU_MEMBER(Point, x, int),
	       GU_MEMBER(Point, y, int));
#else
GU_DEFINE_TYPEDEF(Point, struct,
		  GU_MEMBER(Point, x, int),
		  GU_MEMBER(Point, y, int));
#endif
typedef struct {
	void (*dump)(GuTypeMap* map, GuType* type, void* val);
} Dumper;


static void dump_aux(GuTypeMap* map, GuType* type, void* val)
{
	Dumper* dper = gu_type_map_get(map, type);
	dper->dump(map, type, val);
}

static void dump_int(GuTypeMap* map, GuType* type, void* p) {
	(void) (map && type);
	int* i = p;
	printf("%d\n", *i);
}

static void dump_typedef(GuTypeMap* map, GuType* type, void* p) {
	GuType_typedef* td = (GuType_typedef*)type;
	printf(GU_STRING_FMT "(", GU_STRING_FMT_ARGS(td->name));
	dump_aux(map, td->alias_base.type, p);
	printf(")\n");
}

static void dump_struct(GuTypeMap* map, GuType* type, void* p) {
	GuType_struct* stype = (GuType_struct*)type;
	uint8_t* u = p;
	
	printf("{ ");
	
	for (int i = 0; i < stype->members.len; i++) {
		GuMember* m = &stype->members.elems[i];
		printf(GU_STRING_FMT " = ", GU_STRING_FMT_ARGS(m->name));
		dump_aux(map, m->type, &u[m->offset]);
		printf(",\n");
	}

	printf("}\n");
}

#define DUMPER(k_, f_) { gu_kind(k_), (Dumper[1]){{f_}}}

static GuTypeTable dumpers = 
	GU_TYPETABLE(GU_SLIST_0,
		     DUMPER(int, dump_int),
		     DUMPER(typedef, dump_typedef),
		     DUMPER(struct, dump_struct));


typedef GuVariant Tree;

typedef enum {
	LEAF,
	BRANCH
} TreeTag;


typedef struct {
	int value;
} Leaf;

typedef struct {
	Tree left;
	Tree right;
} Branch;


static void dump_variant(GuTypeMap* map, GuType* type, void* p) {
	GuType_GuVariant* vtype = (GuType_GuVariant*)type;
	GuVariant* v = p;
	GuVariantInfo i = gu_variant_open(*v);
	
	GuConstructor* ctor = &vtype->ctors.elems[i.tag];

	dump_aux(map, ctor->type, i.data);
}

static GU_DECLARE_TYPE(Tree, typedef);

static
GU_DEFINE_TYPEDEF(Leaf, struct,
		  GU_MEMBER(Leaf, value, int));

static
GU_DEFINE_TYPEDEF(Branch, struct,
		  GU_MEMBER(Branch, left, Tree),
		  GU_MEMBER(Branch, right, Tree));

static
GU_DEFINE_TYPEDEF(Tree, GuVariant,
		  GU_CONSTRUCTOR(LEAF, Leaf),
		  GU_CONSTRUCTOR(BRANCH, Branch));

static GuTypeTable dumpers2 = 
	GU_TYPETABLE(GU_SLIST_0,
		     DUMPER(int, dump_int),
		     DUMPER(typedef, dump_typedef),
		     DUMPER(GuVariant, dump_variant),
		     DUMPER(struct, dump_struct));

static void dump(GuType* t, void* val) 
{
	GuPool* pool = gu_pool_new();
	GuTypeMap* map = gu_type_map_new(pool, &dumpers2);
	dump_aux(map, t, val);
}

				   

int main(void)
{
	Point p = { 3, 4 };
	dump(gu_type(Point), &p);

	GuPool* pool = gu_pool_new();
	Tree t1, t2, t3, t4, t5;
	Leaf* l1 = gu_variant_new(pool, LEAF, Leaf, &t1);
	l1->value = 42;
	Leaf* l2 = gu_variant_new(pool, LEAF, Leaf, &t2);
	l2->value = 7;
	Leaf* l3 = gu_variant_new(pool, LEAF, Leaf, &t3);
	l3->value = 16;
	Branch* b1 = gu_variant_new(pool, BRANCH, Branch, &t4);
	b1->left = t2;
	b1->right = t3;
	Branch* b2 = gu_variant_new(pool, BRANCH, Branch, &t5);
	b2->left = t1;
	b2->right = t4;
	dump(gu_type(Tree), &t5);
	return 0;

	
}

