#include <gu/yaml.h>



static const GuString* blotozz = gu_cstring("blotozz");
static GuString* strunk = gu_string("strunk");

static struct {
	const GuString* kreegah;
	const GuString* fnord;
	const GuString* strouh;
} lits = {
	.kreegah = gu_cstring("kreegah"),
	.fnord = gu_cstring("fnord"),
	.strouh = gu_cstring("strouh"),
};


static GU_DEFINE_ATOM(bloh, "bloh");
static GU_DEFINE_ATOM(ugga_mugga, "ugga-mugga");

	
int main(void)
{
	GuPool* pool = gu_pool_new();
	GuYaml* yaml = gu_yaml_new(pool, stdout);

	gu_yaml_begin_mapping(yaml);
	gu_yaml_scalar(yaml, gu_string("fnord"));
	gu_yaml_scalar(yaml, gu_atom(bloh));
	gu_yaml_end(yaml);

	gu_yaml_begin_sequence(yaml);
	gu_yaml_tag_primary(yaml, gu_cstring("strouh"));
	gu_yaml_scalar(yaml, strunk);
	GuYamlAnchor anchor = gu_yaml_anchor(yaml);
	gu_yaml_scalar(yaml, blotozz);
	gu_yaml_alias(yaml, anchor);

	gu_yaml_tag_primary(yaml, lits.kreegah);
	gu_yaml_begin_sequence(yaml);
	gu_yaml_scalar(yaml, lits.fnord);
	gu_yaml_tag_primary(yaml, gu_atom(ugga_mugga));
	gu_yaml_scalar(yaml, gu_cstring("strouhki"));
	gu_yaml_end(yaml);
	gu_yaml_end(yaml);

	gu_pool_free(pool);
	return 0;
}
