#include <gu/yaml.h>
#include <gu/file.h>


static const wchar_t* blotozz = L"blotozz";
static wchar_t* strunk = L"strunk";

static struct {
	const char* kreegah;
	const wchar_t* fnord;
	const char* strouh;
} lits = {
	.kreegah = "kreegah",
	.fnord = L"fnord",
	.strouh = "strouh",
};


static const wchar_t bloh[] = L"bloh";
static const char ugga_mugga[] = "ugga-mugga";
	
int main(void)
{
	GuPool* pool = gu_pool_new();
	GuFile* outf = gu_file(stdout, pool);
	GuYaml* yaml = gu_yaml_new(pool, &outf->wtr);

	gu_yaml_begin_mapping(yaml);
	gu_yaml_scalar(yaml, L"fnord");
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
	gu_yaml_scalar(yaml, L"strouhki");
	gu_yaml_end(yaml);
	gu_yaml_end(yaml);

	gu_pool_free(pool);
	return 0;
}
