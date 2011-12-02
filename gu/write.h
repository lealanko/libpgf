#ifndef GU_WRITE_H_
#define GU_WRITE_H_

#include <gu/error.h>
#include <gu/ucs.h>

typedef const struct GuWriter GuWriter;

struct GuWriter {
	size_t (*write)(GuWriter* self, const GuUCS* buf, size_t size, GuError* err);
	void (*flush)(GuWriter* self, GuError* err);
};

size_t
gu_write(GuWriter* wtr, const GuUCS* buf, size_t size, GuError* err);

void
gu_writer_flush(GuWriter* wtr, GuError* err);

static inline void
gu_ucs_write(GuUCS ucs, GuWriter* wtr, GuError* err)
{
	gu_write(wtr, &ucs, 1, err);
}

void
gu_putc(char c, GuWriter* wtr, GuError* err);

void
gu_puts(const char* str, GuWriter* wtr, GuError* err);

void
gu_vprintf(const char* fmt, va_list args, GuWriter* wtr, GuError* err);

void
gu_printf(GuWriter* wtr, GuError* err, const char* fmt, ...);

GuWriter*
gu_buffered_writer(GuWriter* wtr, size_t buf_size, GuPool* pool);

#include <gu/out.h>

typedef struct GuOutWriter GuOutWriter;

struct GuOutWriter {
	GuWriter wtr;
	GuOut* out;
};

void
gu_out_writer_flush(GuWriter* self, GuError* err);

GuWriter*
gu_char_writer(GuOut* out, GuPool* pool);

GuWriter*
gu_locale_writer(GuOut* out, GuPool* pool);

#endif // GU_WRITE_H_
