#include <gu/yaml.h>
#include <gu/file.h>

#include <mcheck.h>

int main(void)
{
	mtrace();
	
	GuPool* pool = gu_new_pool();
	GuFile* outf = gu_file(stdout, pool);
	GuError* err = gu_error(NULL, type, NULL);
	GuWriter* owtr = gu_locale_writer(&outf->out, pool);
	GuYaml* yaml = gu_new_yaml(owtr, err, pool);

	gu_yaml_begin_mapping(yaml);
	gu_yaml_scalar(yaml, gu_format_string(pool, "foo"));
	gu_yaml_scalar(yaml, gu_format_string(pool, "bar"));
	gu_yaml_end(yaml);

	gu_yaml_begin_sequence(yaml);
	gu_yaml_tag_primary(yaml, "baz");
	gu_yaml_scalar(yaml, gu_format_string(pool, "quux"));
	GuYamlAnchor anchor = gu_yaml_anchor(yaml);
	gu_yaml_scalar(yaml, gu_format_string(pool, "fnarp"));
	gu_yaml_alias(yaml, anchor);

	gu_yaml_tag_primary(yaml, "flurp");
	gu_yaml_begin_sequence(yaml);
	gu_yaml_scalar(yaml, gu_format_string(pool, "drokk"));
	gu_yaml_tag_primary(yaml, "blarg");
	gu_yaml_scalar(yaml, gu_format_string(pool, "strokk"));
	gu_yaml_end(yaml);
	gu_yaml_end(yaml);

	gu_pool_free(pool);
	return 0;
}
