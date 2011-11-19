#ifndef GU_WRITE_H_
#define GU_WRITE_H_

#include <wchar.h>
#include <gu/error.h>

typedef const struct GuWriter GuWriter;

struct GuWriter {
	void (*write)(GuWriter* self, const wchar_t* wcs, size_t size, GuError* err);
};

void
gu_write(GuWriter* wtr, const wchar_t* wcs, size_t size, GuError* err);

void
gu_putc(GuWriter* wtr, wchar_t wc, GuError* err);

void
gu_puts(GuWriter* wtr, const wchar_t* wcs, GuError* err);

void
gu_vwprintf(GuWriter* wtr, const wchar_t* fmt, va_list args, GuError* err);

void
gu_wprintf(GuWriter* wtr, GuError* err, const wchar_t* fmt, ...);

void
gu_vprintf(GuWriter* wtr, const char* fmt, va_list args, GuError* err);

void
gu_printf(GuWriter* wtr, GuError* err, const char* fmt, ...);

#include <gu/seq.h>

GuWriter*
gu_wc_seq_writer(GuWcSeq* wcq, GuPool* pool);

#endif // GU_WRITE_H_
