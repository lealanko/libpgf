#include <gu/print.h>
#include <gu/string.h>

void gu_print_fmt(GuFmt* fmt, const void** args, GuWriter* wtr, GuExn* exn)
{
	for (size_t i = 0; i < fmt->len; i++) {
		GuPrintSpec* spec = &fmt->data[i];
		if (spec->text) {
			gu_puts(spec->text, wtr, exn);
			if (!gu_ok(exn)) return;
		}
		if (args[i]) {
			spec->printer->print(spec->printer, args[i],
					     wtr, exn);
			if (!gu_ok(exn)) return;
		}
	}
}

static void
gu_size_print_fn(GuPrinter* self, const void* p, GuWriter* wtr, GuExn* exn)
{
	(void) self;
	const size_t* sp = p;
	gu_printf(wtr, exn, "%zu", *sp);
}

GuPrinter gu_size_printer[1] = {{ gu_size_print_fn }};

static void
gu_string_print_fn(GuPrinter* self, const void* p, GuWriter* wtr, GuExn* exn)
{
	(void) self;
	const GuString* sp = p;
	gu_string_write(*sp, wtr, exn);
}

GuPrinter gu_string_printer[1] = {{ gu_string_print_fn }};
