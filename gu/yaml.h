#include <gu/string.h>
#include <gu/mem.h>
#include <stdio.h>

typedef struct GuYaml GuYaml;

typedef int GuYamlAnchor;


GuYaml* gu_yaml_new(GuPool* pool, FILE* out);

GuYamlAnchor gu_yaml_anchor(GuYaml* yaml);

void gu_yaml_tag_primary(GuYaml* yaml, const GuString* tag);
void gu_yaml_tag_secondary(GuYaml* yaml, const GuString* tag);
void gu_yaml_tag_named(GuYaml* yaml, const GuString* handle, const GuString* tag);
void gu_yaml_tag_verbatim(GuYaml* yaml, const GuString* uri);
void gu_yaml_tag_non_specific(GuYaml* yaml);


void gu_yaml_scalar(GuYaml* yaml, const GuString* str);
void gu_yaml_alias(GuYaml* yaml, GuYamlAnchor anchor);

void gu_yaml_begin_document(GuYaml* yaml);

void gu_yaml_begin_sequence(GuYaml* yaml);

void gu_yaml_begin_mapping(GuYaml* yaml);

void gu_yaml_end(GuYaml* yaml);

