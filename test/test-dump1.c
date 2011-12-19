#include <gu/dump.h>
#include <gu/type.h>
#include <gu/str.h>
#include <gu/variant.h>
#include <gu/map.h>

typedef GuList(int) Ints;

static GU_DEFINE_TYPE(Ints, GuList, gu_type(int));

typedef struct {
	int foo;
	GuStr bar;
	Ints* fooh;
} Baz;

GU_DEFINE_TYPE(
	Baz, struct,
	GU_MEMBER(Baz, foo, int),
	GU_MEMBER_V(Baz, bar, gu_type(GuStr)),
	GU_MEMBER_V(Baz, fooh, gu_ptr_type(Ints)));

typedef GuMap Dict;

// todo: sharing
GU_DEFINE_TYPE(Dict, GuMap, gu_type(GuStr), gu_str_hasher, gu_type(GuStr),
	       &gu_null_str);

typedef struct {
	GuStr koo;
	GuLength len;
	int ints[];
} Blump;

GU_DEFINE_TYPE(Blump, struct,
	       GU_MEMBER(Blump, koo, GuStr),
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

GU_DEFINE_TYPE(TreeTag, enum,
	       GU_ENUM_C(TreeTag, LEAF),
	       GU_ENUM_C(TreeTag, BRANCH));

GU_DEFINE_TYPE(Branch, struct,
	       GU_MEMBER(Branch, left, Tree),
	       GU_MEMBER(Branch, right, Tree));

GU_DEFINE_TYPE(Leaf, struct,
	       GU_MEMBER(Leaf, val, int));

GU_DEFINE_TYPE(Tree, GuVariant,
	       GU_CONSTRUCTOR_S(LEAF, Leaf,
				GU_MEMBER(Leaf, val, int)),
	       GU_CONSTRUCTOR_S(BRANCH, Branch,
				GU_MEMBER(Branch, left, Tree),
				GU_MEMBER(Branch, right, Tree)));

int main(void)
{
	GuPool* pool = gu_new_pool();
	Ints* fooh = gu_list_new(Ints, pool, 3);
	Baz b = { 42, "fnord", fooh };
	int* elems = gu_list_elems(fooh);
	elems[0] = 7;
	elems[1] = 99;
	elems[2] = 623;
	GuDump* ctx = gu_new_dump(pool, stdout, NULL);
	gu_dump(gu_type(Baz), &b, ctx);
	
	Dict* dict = gu_new_map(GuStr, gu_str_hasher, GuStr, NULL, pool);
	GuStr fnord = "fnord";
	gu_map_put(dict, &(int){42}, GuStr, "fnord");
	gu_map_put(dict, &(int){7}, GuStr, "blurh");
	gu_map_put(dict, &(int){11}, GuStr, fnord);
	gu_map_put(dict, &(int){15}, GuStr, fnord);
	gu_dump(gu_type(Dict), dict, ctx);


	Blump* blump = gu_flex_new(pool, Blump, ints, 8);
	blump->len = 8;
	blump->koo = "urk";
	blump->ints[0] = 42;
	blump->ints[1] = 25;
	blump->ints[7] = 77;
	gu_dump(gu_type(Blump), blump, ctx);

	Tree leaf = gu_variant_new_s(pool, LEAF, Leaf, 42);
	Tree branch = gu_variant_new_s(pool, BRANCH, Branch, leaf, leaf);
	gu_dump(gu_type(Tree), &branch, ctx);

	TreeTag tag = BRANCH;
	gu_dump(gu_type(TreeTag), &tag, ctx);

	gu_pool_free(pool);
	return 0;
}




