#include <gu/dump.h>
#include <gu/type.h>


typedef struct {
	int foo;
	GuString* bar;
} Baz;

GU_DEFINE_TYPE(
	Baz, struct,
	GU_MEMBER(Baz, foo, int),
	GU_MEMBER_V(Baz, bar, 
		    GU_TYPE_LIT(pointer, GuString*,
				gu_type(GuString))));


typedef GuIntMap Dict;

GU_DEFINE_TYPE(Dict, GuIntMap, gu_type(GuString));


int main(void)
{
	Baz b = { 42, gu_string("fnord") };
	GuPool* pool = gu_pool_new();
	GuTypeMap* tmap = gu_type_map_new(pool, &gu_dump_table);
	GuYaml* yaml = gu_yaml_new(pool, stdout);
	GuDumpCtx ctx = {
		.pool = pool,
		.dumpers = tmap,
		.data = NULL,
		.yaml = yaml 
	};
	gu_dump(gu_type(Baz), &b, &ctx);
	
	Dict* dict = gu_intmap_new(pool);
	gu_intmap_set(dict, 42, gu_string("fnord"));
	gu_intmap_set(dict, 7, gu_string("blurh"));
	gu_dump(gu_type(Dict), dict, &ctx);

	gu_pool_free(pool);
	return 0;
}




