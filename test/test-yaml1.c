#include <gu/yaml.h>



static const char* blotozz = "blotozz";
static char* strunk = "strunk";

static struct {
	const char* kreegah;
	const char* fnord;
	const char* strouh;
} lits = {
	.kreegah = "kreegah",
	.fnord = "fnord",
	.strouh = "strouh",
};


static const char bloh[] = "bloh";
static const char ugga_mugga[] = "ugga-mugga";
	
int main(void)
{
	GuPool* pool = gu_pool_new();
	GuYaml* yaml = gu_yaml_new(pool, stdout);

	gu_yaml_begin_mapping(yaml);
	gu_yaml_scalar(yaml, "fnord");
	gu_yaml_scalar(yaml, bloh);
	gu_yaml_end(yaml);

	gu_yaml_begin_sequence(yaml);
	gu_yaml_tag_primary(yaml, "strouh");
	gu_yaml_scalar(yaml, strunk);
	GuYamlAnchor anchor = gu_yaml_anchor(yaml);
	gu_yaml_scalar(yaml, blotozz);
	gu_yaml_alias(yaml, anchor);

	gu_yaml_tag_primary(yaml, lits.kreegah);
	gu_yaml_begin_sequence(yaml);
	gu_yaml_scalar(yaml, lits.fnord);
	gu_yaml_tag_primary(yaml, ugga_mugga);
	gu_yaml_scalar(yaml, "strouhki");
	gu_yaml_end(yaml);
	gu_yaml_end(yaml);

	gu_pool_free(pool);
	return 0;
}
