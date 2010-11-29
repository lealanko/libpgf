
#include <stdio.h>
#include <gu/type.h>
#include <gu/variant.h>
#include <gu/mem.h>

typedef struct {
	int x;
	int y;
} Point;

GU_DEFINE_TYPE(Point, _struct, 
	       GU_MEMBER(Point, x, int),
	       GU_MEMBER(Point, y, int));

typedef struct {
	void (*dump)(GuTypeMap* map, GuType* type, void* val);
} Dumper;

static void dump_aux(GuTypeMap* map, GuType* type, void* val)
{
	Dumper* dper = gu_type_map_lookup(map, type);
	dper->dump(map, type, val);
}

static void dump_int(GuTypeMap* map, GuType* type, void* p) {
	int* i = p;
	printf("%d\n", *i);
}

static void dump_struct(GuTypeMap* map, GuType* type, void* p) {
	GuType__struct* stype = (GuType__struct*)type;
	uint8_t* u = p;
	
	printf("%s { ", stype->named.name);
	
	for (int i = 0; i < stype->members.len; i++) {
		GuMember* m = &stype->members.elems[i];
		printf("%s = ", m->name);
		dump_aux(map, m->type, &u[m->offset]);
		printf(",\n");
	}

	printf("}\n");
}

static void dump_variant(GuTypeMap* map, GuType* type, void* p) {
	GuType_GuVariant* vtype = (GuType_GuVariant*)type;
	GuVariant* v = p;
	GuVariantInfo i = gu_variant_open(*v);
	
	GuConstructor* ctor = &vtype->ctors.elems[i.tag];

	dump_aux(map, ctor->type, i.data);
}


static GuTypeTable dumpers = 
	GU_TYPETABLE(GU_SLIST_0,
		     { GU_KIND_P(int), (Dumper[]){{dump_int}}},
		     { GU_KIND_P(_struct), (Dumper[]){{dump_struct}}});


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

static GU_DECLARE_TYPE(Tree, GuVariant);

static
GU_DEFINE_TYPE(Leaf, _struct, 
	       GU_MEMBER(Leaf, value, int));

static
GU_DEFINE_TYPE(Branch, _struct,
	       GU_MEMBER(Branch, left, Tree),
	       GU_MEMBER(Branch, right, Tree));

#if 1
static
GU_DEFINE_TYPE(Tree, GuVariant,
	       GU_CONSTRUCTOR_S(LEAF, Leaf,
				GU_MEMBER(Leaf, value, int)),
	       GU_CONSTRUCTOR(BRANCH, Branch));
#else
static
GU_DEFINE_TYPE(Tree, GuVariant,
	       GU_CONSTRUCTOR(LEAF, Leaf),
	       GU_CONSTRUCTOR(BRANCH, Branch));
#endif

static GuTypeTable dumpers2 = 
	GU_TYPETABLE(GU_SLIST_0,
		     { GU_KIND_P(int), (Dumper[]){{dump_int}}},
		     { GU_KIND_P(_struct), (Dumper[]){{dump_struct}}},
		     { GU_KIND_P(GuVariant), (Dumper[]){{dump_variant}}});

static void dump(GuType* t, void* val) 
{
	GuPool* pool = gu_pool_new();
	GuTypeMap* map = gu_type_map_new(pool, &dumpers2);
	dump_aux(map, t, val);
}


				   

int main(void)
{
	Point p = { 3, 4 };
	dump(GU_TYPE_P(Point), &p);

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
	dump(GU_TYPE_P(Tree), &t5);
	return 0;

	
}

