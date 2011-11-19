#ifndef GU_YAML_H_
#define GU_YAML_H_

#include <gu/mem.h>
#include <gu/write.h>

typedef struct GuYaml GuYaml;

typedef int GuYamlAnchor;


GuYaml* gu_yaml_new(GuPool* pool, GuWriter* wtr);

GuYamlAnchor gu_yaml_anchor(GuYaml* yaml);

void gu_yaml_tag_primary(GuYaml* yaml, const char* tag);
void gu_yaml_tag_secondary(GuYaml* yaml, const char* tag);
void gu_yaml_tag_named(GuYaml* yaml, const char* handle, const char* tag);
void gu_yaml_tag_verbatim(GuYaml* yaml, const char* uri);
void gu_yaml_tag_non_specific(GuYaml* yaml);
void gu_yaml_comment(GuYaml* yaml, const wchar_t* comment);


void gu_yaml_scalar(GuYaml* yaml, const wchar_t* str);

void gu_yaml_alias(GuYaml* yaml, GuYamlAnchor anchor);

void gu_yaml_begin_document(GuYaml* yaml);

void gu_yaml_begin_sequence(GuYaml* yaml);

void gu_yaml_begin_mapping(GuYaml* yaml);

void gu_yaml_end(GuYaml* yaml);

#endif // GU_YAML_H_
