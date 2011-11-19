#include <gu/yaml.h>
#include <gu/seq.h>
#include <gu/assert.h>
#include <gu/read.h>
#include <gu/ucs.h>
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
	GuWriter* wtr;
	GuError* err;
	GuPool* pool;
	GuYamlState* state;
	bool in_node;
	bool have_anchor;
	bool have_tag; 
	int next_anchor;
	bool indent;
	int indent_level;
	bool indented;
	GuYamlStack stack;
};


GuYaml*
gu_yaml_new(GuPool* pool, GuWriter* wtr)
{
	GuYaml* yaml = gu_new(pool, GuYaml);
	yaml->wtr = wtr;
	yaml->pool = pool;
	yaml->err = gu_error_new(pool);
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
gu_yaml_printf(GuYaml* yaml, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	gu_vprintf(yaml->wtr, fmt, args, yaml->err);
	va_end(args);
}

static void
gu_yaml_putc(GuYaml* yaml, char c)
{
	gu_yaml_printf(yaml, "%c", c);
}

static void
gu_yaml_puts(GuYaml* yaml, const char* str)
{
	gu_yaml_printf(yaml, "%s", str);
}

static void
gu_yaml_begin_line(GuYaml* yaml)
{
	if (yaml->indent && !yaml->indented) {
		for (int i = 0; i < yaml->indent_level; i++) {
			gu_yaml_putc(yaml, L' ');
		}
		yaml->indented = true;
	}
}

static void
gu_yaml_end_line(GuYaml* yaml)
{
	if (yaml->indent) {
		gu_yaml_putc(yaml, L'\n');
	}
	yaml->indented = false;
}


static void 
gu_yaml_begin_node(GuYaml* yaml)
{
	gu_yaml_begin_line(yaml);
	if (!yaml->in_node) {
		if (yaml->state->prefix != NULL) {
			gu_yaml_puts(yaml, yaml->state->prefix);
		}
		yaml->in_node = true;
	}
}

static void
gu_yaml_end_node(GuYaml* yaml)
{
	gu_assert(yaml->in_node);
	if (yaml->state->suffix != NULL) {
		gu_yaml_puts(yaml, yaml->state->suffix);
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
	gu_yaml_puts(yaml, klass->open);
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
	gu_yaml_puts(yaml, f.klass->close);
	yaml->state = f.next;
	yaml->in_node = true;
	gu_yaml_end_node(yaml);
}


void 
gu_yaml_scalar(GuYaml* yaml, const wchar_t* wcs)
{
	gu_yaml_begin_node(yaml);
	gu_yaml_putc(yaml, '"');
	GuPool* tmp_pool = gu_pool_new();
	GuReader* rdr = gu_wcs_reader(wcs, wcslen(wcs), tmp_pool);

	static const char esc[0x20] = {
		[0x00] = '0',
		[0x07] = 'a', 'b', 't', 'n', 'v', 'f', 'r',
		[0x1b] = 'e'
	};

	while (true) {
		GuUcs u = gu_read_ucs(rdr, yaml->err);
		if (u == GU_UCS_EOF) {
			break;
		}
		if (u == 0x22 || u == 0x5c) {
			gu_yaml_putc(yaml, L'\\');
			gu_write_ucs(yaml->wtr, u, yaml->err);
		} else if (u < 0x20 && esc[u]) {
			gu_yaml_printf(yaml, "\\%c", esc[u]);
		} else if (u >= 0x7f && u <= 0xff) {
			gu_yaml_printf(yaml, "\\x%02x", (unsigned) u);
		} else if ((u >= 0xd800 && u <= 0xdfff) ||
			   (u >= 0xfffe && u <= 0xffff)) {
			gu_yaml_printf(yaml, "\\u%04x", (unsigned) u);
		} else {
			gu_write_ucs(yaml->wtr, u, yaml->err);
		}
	}
	gu_yaml_putc(yaml, L'"');
	gu_yaml_end_node(yaml);
}

static void 
gu_yaml_tag(GuYaml* yaml, const char* format, ...)
{
	gu_yaml_begin_node(yaml);
	gu_assert(!yaml->have_tag);
	gu_yaml_putc(yaml, L'!');
	va_list args;
	va_start(args, format);
	gu_vprintf(yaml->wtr, format, args, yaml->err);
	va_end(args);
	gu_yaml_putc(yaml, L' ');
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
	gu_yaml_printf(yaml, "&%d ", anchor);
	return anchor;
}

void
gu_yaml_alias(GuYaml* yaml, GuYamlAnchor anchor)
{
	gu_yaml_begin_node(yaml);
	gu_assert(!yaml->have_anchor && !yaml->have_tag);
	gu_yaml_printf(yaml, "*%d ", anchor);
	gu_yaml_end_node(yaml);
	return;
}

void gu_yaml_comment(GuYaml* yaml, const wchar_t* comment)
{
	gu_yaml_begin_line(yaml);
	// TODO: verify no newlines in comment
	gu_yaml_printf(yaml, "# %ls\n", comment);
	yaml->indented = false;
}

