#include <gu/dump.h>
#include <gu/type.h>
#include <gu/variant.h>


typedef GuList(int) Ints;

static GU_DEFINE_TYPE(Ints, GuList, int);

typedef struct {
	int foo;
	GuString* bar;
	Ints* fooh;
} Baz;

GU_DEFINE_TYPE(
	Baz, struct,
	GU_MEMBER(Baz, foo, int),
	GU_MEMBER_V(Baz, bar, 
		    GU_TYPE_LIT(pointer, GuString*,
				gu_type(GuString))),
	GU_MEMBER_V(Baz, fooh,
		    GU_TYPE_LIT(pointer, Ints*,
				gu_type(Ints)))
);


typedef GuIntMap Dict;

GU_DEFINE_TYPE(Dict, GuIntMap, 
	       GU_TYPE_LIT(shared, GuString*,
			   gu_type(GuString)));

typedef struct {
	GuString* koo;
	GuLength len;
	int ints[];
} Blump;

GU_DEFINE_TYPE(Blump, struct,
	       GU_MEMBER_V(Blump, koo, GU_TYPE_LIT(pointer, GuString*, gu_type(GuString))),
	       GU_MEMBER(Blump, len, GuLength),
	       GU_FLEX_MEMBER(Blump, ints, int));

typedef GuVariant Tree;
GU_DECLARE_TYPE(Tree, GuVariant);

typedef enum {
	LEAF,
	BRANCH
} TreeTag;

typedef struct {
	Tree left;
	Tree right;
} Branch;

typedef struct {
	int val;
} Leaf;


GU_DEFINE_TYPE(Branch, struct,
	       GU_MEMBER(Branch, left, Tree),
	       GU_MEMBER(Branch, right, Tree));

GU_DEFINE_TYPE(Leaf, struct,
	       GU_MEMBER(Leaf, val, int));

GU_DEFINE_TYPE(Tree, GuVariant,
	       GU_CONSTRUCTOR(LEAF, Leaf),
	       GU_CONSTRUCTOR(BRANCH, Branch));

int main(void)
{
	GuPool* pool = gu_pool_new();
	Ints* fooh = gu_list_new(Ints, pool, 3);
	Baz b = { 42, gu_string("fnord"), fooh };
	int* elems = gu_list_elems(fooh);
	elems[0] = 7;
	elems[1] = 99;
	elems[2] = 623;
	GuTypeMap* tmap = gu_type_map_new(pool, &gu_dump_table);
	GuYaml* yaml = gu_yaml_new(pool, stdout);
	GuDumpCtx ctx = {
		.pool = pool,
		.dumpers = tmap,
		.data = NULL,
		.yaml = yaml 
	};
	ctx.data = gu_map_new(pool, NULL, NULL);
	gu_dump(gu_type(Baz), &b, &ctx);
	
	Dict* dict = gu_intmap_new(pool);
	GuString* fnord = gu_string("fnord");
	gu_intmap_set(dict, 42, gu_string("fnord"));
	gu_intmap_set(dict, 7, gu_string("blurh"));
	gu_intmap_set(dict, 11, fnord);
	gu_intmap_set(dict, 15, fnord);
	gu_dump(gu_type(Dict), dict, &ctx);


	Blump* blump = gu_flex_new(pool, Blump, ints, 8);
	blump->len = 8;
	blump->koo = gu_string("urk");
	blump->ints[0] = 42;
	blump->ints[1] = 25;
	blump->ints[7] = 77;
	gu_dump(gu_type(Blump), blump, &ctx);

	Tree leaf = gu_variant_new_s(pool, LEAF, Leaf, 42);
	Tree branch = gu_variant_new_s(pool, BRANCH, Branch, leaf, leaf);
	gu_dump(gu_type(Tree), &branch, &ctx);

	gu_pool_free(pool);
	return 0;
}




