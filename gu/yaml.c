#include <gu/yaml.h>
#include <gu/seq.h>
#include <gu/assert.h>
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

GU_SEQ_DEFINE(GuYamlStack, gu_yaml_stack, GuYamlFrame);

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
	bool indented;
	GuYamlStack* stack;
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
	yaml->stack = gu_yaml_stack_new(pool);
	yaml->indent = true;
	yaml->indent_level = 0;
	yaml->indented = false;
	return yaml;
}


static void
gu_yaml_begin_line(GuYaml* yaml)
{
	if (yaml->indent && !yaml->indented) {
		for (int i = 0; i < yaml->indent_level; i++) {
			fputc(' ', yaml->out);
		}
		yaml->indented = true;
	}
}

static void
gu_yaml_end_line(GuYaml* yaml)
{
	if (yaml->indent) {
		fputc('\n', yaml->out);
	}
	yaml->indented = false;
}


static void 
gu_yaml_begin_node(GuYaml* yaml)
{
	gu_yaml_begin_line(yaml);
	if (!yaml->in_node) {
		if (yaml->state->prefix != NULL) {
			fputs(yaml->state->prefix, yaml->out);
		}
		yaml->in_node = true;
	}
}

static void
gu_yaml_end_node(GuYaml* yaml)
{
	gu_assert(yaml->in_node);
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
	gu_yaml_stack_push(yaml->stack, (GuYamlFrame)
			   { .klass = klass, .next = yaml->state});
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
	gu_assert(!yaml->in_node);
	yaml->indent_level--;
	gu_yaml_begin_line(yaml);
	GuYamlFrame f = gu_yaml_stack_pop(yaml->stack);
	fputs(f.klass->close, yaml->out);
	yaml->state = f.next;
	yaml->in_node = true;
	gu_yaml_end_node(yaml);
}


void 
gu_yaml_scalar(GuYaml* yaml, const char* str)
{
	gu_yaml_begin_node(yaml);
	fputc('"', yaml->out);
	const char* buf = str ? str : "";
	int span_start = 0;
	int i = 0;
	while (true) {
		while (true) {
			char c = buf[i];
			if (!c || c == '"' || c == '\\' || !isprint(c)) {
				break;
			}
			i++;
		}
		fwrite(&buf[span_start], 1, i - span_start, yaml->out);
		if (buf[i]) {
			span_start = i + 1;
			// XXX: platform-dependent encoding
			fprintf(yaml->out, "\\%02x", (unsigned char) buf[i]);
			i++;
		} else {
			break;
		}
	}
	fputc('"', yaml->out);
	gu_yaml_end_node(yaml);
}

static void 
gu_yaml_tag(GuYaml* yaml, const char* format, ...)
{
	gu_yaml_begin_node(yaml);
	gu_assert(!yaml->have_tag);
	fputc('!', yaml->out);
	va_list args;
	va_start(args, format);
	vfprintf(yaml->out, format, args);
	va_end(args);
	fputc(' ', yaml->out);
	yaml->have_tag = true;
}

void 
gu_yaml_tag_primary(GuYaml* yaml, const char* tag) 
{
	// TODO: check tag validity
	gu_yaml_tag(yaml, "%s", tag);
}

void
gu_yaml_tag_secondary(GuYaml* yaml, const char* tag)
{
	// TODO: check tag validity
	gu_yaml_tag(yaml, "!%s", tag);
}

void
gu_yaml_tag_named(GuYaml* yaml, const char* handle, const char* tag)
{
	// TODO: check tag validity
	gu_yaml_tag(yaml, "%s!%s", handle, tag);
}

void
gu_yaml_tag_verbatim(GuYaml* yaml, const char* uri)
{
	// XXX: uri escaping?
	gu_yaml_tag(yaml, "<%s>", uri);
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
	gu_assert(!yaml->have_anchor);
	yaml->have_anchor = true;
	int anchor = yaml->next_anchor++;
	fprintf(yaml->out, "&%d ", anchor);
	return anchor;
}

void
gu_yaml_alias(GuYaml* yaml, GuYamlAnchor anchor)
{
	gu_yaml_begin_node(yaml);
	gu_assert(!yaml->have_anchor && !yaml->have_tag);
	fprintf(yaml->out, "*%d", anchor);
	gu_yaml_end_node(yaml);
	return;
}

void gu_yaml_comment(GuYaml* yaml, const char* comment)
{
	gu_yaml_begin_line(yaml);
	// XXX: escaping?
	fprintf(yaml->out, "# %s\n", comment);
	yaml->indented = false;
}

