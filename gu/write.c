#include <gu/write.h>


void
gu_write(GuWriter* wtr, const wchar_t* wcs, size_t size, GuError* err)
{
	wtr->write(wtr, wcs, size, err);
}

void
gu_putc(GuWriter* wtr, wchar_t wc, GuError* err)
{
	gu_write(wtr, &wc, 1, err);
}

void
gu_puts(GuWriter* wtr, const wchar_t* wcs, GuError* err)
{
	gu_write(wtr, wcs, wcslen(wcs), err);
}

typedef struct GuFormatError GuFormatError;

struct GuFormatError {
	const wchar_t* fmt;
};

GU_DEFINE_TYPE(GuFormatError, abstract, _);

void
gu_vwprintf(GuWriter* wtr, const wchar_t* fmt, va_list args, GuError* err)
{
	GuPool* tmp_pool = gu_pool_new();
	wchar_t* wcs = gu_vaswprintf(fmt, args, tmp_pool);
	if (!wcs) {
		gu_raise(err, GuFormatError, 
			 .fmt = gu_wcsdup(fmt, gu_error_pool(err)));
	}
	gu_puts(wtr, wcs, err);
	gu_pool_free(tmp_pool);
}

void
gu_wprintf(GuWriter* wtr, GuError* err, const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	gu_vwprintf(wtr, fmt, args, err);
	va_end(args);
}

void
gu_vprintf(GuWriter* wtr, const char* fmt, va_list args, GuError* err)
{
	GuPool* tmp_pool = gu_pool_new();
	wchar_t* wfmt = gu_str_wcs(fmt, tmp_pool);
	gu_vwprintf(wtr, wfmt, args, err);
	gu_pool_free(tmp_pool);
}

void
gu_printf(GuWriter* wtr, GuError* err, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	gu_vprintf(wtr, fmt, args, err);
	va_end(args);
}


