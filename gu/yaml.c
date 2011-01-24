#include "yaml.h"
#include <gu/stack.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

typedef const struct GuYamlState GuYamlState;

struct GuYamlState {
	const char* prefix;
	const char* suffix;
	GuYamlState* next;
};

static const struct {
	GuYamlState document, first_key, key, value, first_elem, elem;
} gu_yaml_states = {
	.document = {
		.prefix = "---\n",
		.suffix = "\n...\n",
		.next = &gu_yaml_states.document,
	},
	.key = {
		.prefix = "? ",
		.next = &gu_yaml_states.value,
	},
	.value = {
		.prefix = ": ",
		.suffix = ",",
		.next = &gu_yaml_states.key,
	},
	.elem = {
		.suffix = ",",
		.next = &gu_yaml_states.elem,
	},
};

typedef const struct GuYamlFrameClass GuYamlFrameClass;

struct GuYamlFrameClass {
	const char* open;
	GuYamlState* first;
	const char* close;
};

static const struct {
	GuYamlFrameClass document, mapping, sequence;
} gu_yaml_frame_classes = {
	.mapping = {
		.open = "{",
		.first = &gu_yaml_states.key,
		.close = "}",
	},
	.sequence = {
		.open = "[",
		.first = &gu_yaml_states.elem,
		.close = "]",
	},
};

typedef struct GuYamlFrame GuYamlFrame;

struct GuYamlFrame {
	GuYamlFrameClass* klass;
	GuYamlState* next;
};

struct GuYaml {
	FILE* out;
	GuPool* pool;
	GuYamlState* state;
	bool in_node;
	bool have_anchor;
	bool have_tag; 
	int next_anchor;
	bool indent;
	int indent_level;
	GuStack* stack;
};


GuYaml*
gu_yaml_new(GuPool* pool, FILE* out)
{
	GuYaml* yaml = gu_new(pool, GuYaml);
	yaml->out = out;
	yaml->pool = pool;
	yaml->state = &gu_yaml_states.document;
	yaml->in_node = false;
	yaml->have_anchor = false;
	yaml->have_tag = false;
	yaml->next_anchor = 1;
	yaml->stack = gu_stack_new(pool, GuYamlFrame);
	yaml->indent = true;
	yaml->indent_level = 0;
	return yaml;
}


static void
gu_yaml_begin_line(GuYaml* yaml)
{
	if (yaml->indent) {
		for (int i = 0; i < yaml->indent_level; i++) {
			fputc(' ', yaml->out);
		}
	}
}

static void
gu_yaml_end_line(GuYaml* yaml)
{
	if (yaml->indent) {
		fputc('\n', yaml->out);
	}
}


static void 
gu_yaml_begin_node(GuYaml* yaml)
{
	if (!yaml->in_node) {
		gu_yaml_begin_line(yaml);
		if (yaml->state->prefix != NULL) {
			fputs(yaml->state->prefix, yaml->out);
		}
		yaml->in_node = true;
	}
}

static void
gu_yaml_end_node(GuYaml* yaml)
{
	g_assert(yaml->in_node);
	if (yaml->state->suffix != NULL) {
		fputs(yaml->state->suffix, yaml->out);
	}
	gu_yaml_end_line(yaml);
	yaml->in_node = false;
	yaml->have_anchor = false;
	yaml->have_tag = false;
	if (yaml->state != NULL) {
		yaml->state = yaml->state->next;
	}
}

static void
gu_yaml_begin(GuYaml* yaml, GuYamlFrameClass* klass)
{
	gu_yaml_begin_node(yaml);
	fputs(klass->open, yaml->out);
	GuYamlFrame* frame = gu_stack_push(yaml->stack);
	frame->klass = klass;
	frame->next = yaml->state;
	yaml->state = klass->first;
	yaml->in_node = yaml->have_anchor = yaml->have_tag = false;
	gu_yaml_end_line(yaml);
	yaml->indent_level++;
}

void
gu_yaml_begin_mapping(GuYaml* yaml)
{
	gu_yaml_begin(yaml, &gu_yaml_frame_classes.mapping);
}

void
gu_yaml_begin_sequence(GuYaml* yaml)
{
	gu_yaml_begin(yaml, &gu_yaml_frame_classes.sequence);
}

void 
gu_yaml_end(GuYaml* yaml)
{
	g_assert(!yaml->in_node);
	yaml->indent_level--;
	gu_yaml_begin_line(yaml);
	GuYamlFrame* f = gu_stack_peek(yaml->stack);
	fputs(f->klass->close, yaml->out);
	yaml->state = f->next;
	gu_stack_pop(yaml->stack);
	yaml->in_node = true;
	gu_yaml_end_node(yaml);
}


void 
gu_yaml_scalar(GuYaml* yaml, const GuString* str)
{
	gu_yaml_begin_node(yaml);
	fputc('"', yaml->out);
	int span_start = 0;
	const char* buf = gu_string_cdata(str);
	int len = gu_string_length(str);
	for (int i = 0; i < len; i++) {
		char c = buf[i];
		if (c == '"' || c == '\\' || !isprint(c)) {
			fwrite(&buf[span_start], 1, i - span_start, yaml->out);
			span_start = i + 1;
			fprintf(yaml->out, "\\%02x", (unsigned char) c);
		}
	}
	fwrite(&buf[span_start], 1, len - span_start, yaml->out);
	fputc('"', yaml->out);
	gu_yaml_end_node(yaml);
}

static void 
gu_yaml_tag(GuYaml* yaml, const char* format, ...)
{
	gu_yaml_begin_node(yaml);
	g_assert(!yaml->have_tag);
	fputc('!', yaml->out);
	va_list args;
	va_start(args, format);
	vfprintf(yaml->out, format, args);
	va_end(args);
	fputc(' ', yaml->out);
	yaml->have_tag = true;
}

void 
gu_yaml_tag_primary(GuYaml* yaml, const GuString* tag) 
{
	// TODO: check tag validity
	gu_yaml_tag(yaml, GU_STRING_FMT, GU_STRING_FMT_ARGS(tag));
}

void
gu_yaml_tag_secondary(GuYaml* yaml, const GuString* tag)
{
	// TODO: check tag validity
	gu_yaml_tag(yaml, "!" GU_STRING_FMT, GU_STRING_FMT_ARGS(tag));
}

void
gu_yaml_tag_named(GuYaml* yaml, const GuString* handle, const GuString* tag)
{
	// TODO: check tag validity
	gu_yaml_tag(yaml, GU_STRING_FMT "!" GU_STRING_FMT,
		    GU_STRING_FMT_ARGS(handle), GU_STRING_FMT_ARGS(tag));
}

void
gu_yaml_tag_verbatim(GuYaml* yaml, const GuString* uri)
{
	gu_yaml_tag(yaml, "<" GU_STRING_FMT ">",
		    GU_STRING_FMT_ARGS(uri));
}

void
gu_yaml_tag_non_specific(GuYaml* yaml)
{
	gu_yaml_tag(yaml, "");
}

GuYamlAnchor
gu_yaml_anchor(GuYaml* yaml)
{
	gu_yaml_begin_node(yaml);
	g_assert(!yaml->have_anchor);
	yaml->have_anchor = true;
	int anchor = yaml->next_anchor++;
	fprintf(yaml->out, "&%d ", anchor);
	return anchor;
}

void
gu_yaml_alias(GuYaml* yaml, GuYamlAnchor anchor)
{
	gu_yaml_begin_node(yaml);
	g_assert(!yaml->have_anchor && !yaml->have_tag);
	fprintf(yaml->out, "*%d", anchor);
	gu_yaml_end_node(yaml);
	return;
}