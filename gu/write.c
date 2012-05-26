#include <gu/write.h>
#include <gu/str.h>

size_t
gu_utf32_write(const GuUCS* src, size_t len, GuWriter* wtr, GuExn* err)
{
	return gu_utf32_out_utf8(src, len, &wtr->out_, err);
}


void
gu_vprintf(const char* fmt, va_list args, GuWriter* wtr, GuExn* err)
{
	GuPool* tmp_pool = gu_local_pool();
	char* str = gu_vasprintf(fmt, args, tmp_pool);
	gu_puts(str, wtr, err);
	gu_pool_free(tmp_pool);
}

void
gu_printf(GuWriter* wtr, GuExn* err, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	gu_vprintf(fmt, args, wtr, err);
	va_end(args);
}

GuWriter*
gu_new_writer(GuOutStream* utf8_stream, GuPool* pool)
{
	GuWriter* wtr = gu_new(GuWriter, pool);
	wtr->out_ = gu_init_out(utf8_stream);
	return wtr;
}


GuWriter*
gu_new_utf8_writer(GuOut* utf8_out, GuPool* pool)
{
	GuOutStream* stream = gu_out_proxy_stream(utf8_out, pool);
	return gu_new_writer(stream, pool);
}


#ifdef GU_UCS_WCHAR
#include <stdlib.h>
#include <wchar.h>
#endif

#define GU_LOCALE_WRITER_BUF_SIZE 4096

typedef struct GuLocaleOutStream GuLocaleOutStream;

struct GuLocaleOutStream {
	GuOutStream stream;
	GuOut* out;
#ifdef GU_UCS_WCHAR
	mbstate_t ps;
#endif
	GuUCS* ucs_buf_curr;
	GuUCS ucs_buf[GU_LOCALE_WRITER_BUF_SIZE + 1];
};

static void
gu_locale_writer_flush(GuOutStream* os, GuExn* err)
{
	GuLocaleOutStream* los = (GuLocaleOutStream*) os;
	const GuUCS* src_curr = los->ucs_buf;
	GuUCS* src_end = los->ucs_buf_curr;
	*src_end = L'\0';
	while (src_curr < src_end) {
		size_t sz;
		// Need at least two bytes: one for proper output, one for
		// the '\0' that wcsrtombs insists on.
		uint8_t* span = gu_out_begin_span(los->out, 2, &sz, err);
#ifdef GU_UCS_WCHAR
		const GuUCS* src_orig = src_curr;
		const GuUCS* next = &src_curr[wcslen(src_curr)];
		if (!gu_ok(err)) return;
		size_t n = wcsrtombs((char*) span, &src_curr, sz, &los->ps);
		while (n == (size_t) -1) {
			// Encoding error. Just replace the offending
			// character and try again.
			*(GuUCS*) src_curr = L'?';
			src_curr = src_orig;
			n = wcsrtombs((char*) span, &src_curr, sz,
				      &los->ps);
		}
		if (src_curr == NULL) {
			src_curr = next + 1;
			if (src_curr <= src_end) {
				n++;
			} 
		}
#else
		size_t n = GU_MIN(sz, (size_t) (src_end - src_curr));
		for (size_t i = 0; i < n; i++) {
			GuUCS u = src_curr[i];
			char c = gu_ucs_char(u);
			if (c == '\0' && u != 0) {
				c = '?';
			}
			span[i] = (uint8_t) c;
		}
		src_curr += n;
#endif
		gu_out_end_span(los->out, span, n, err);
		if (!gu_ok(err)) return;
	}
	los->ucs_buf_curr = los->ucs_buf;
}

size_t
gu_locale_writer_output(GuOutStream* os, const uint8_t* utf8_src, size_t sz, 
			GuExn* exn)
{
	GuLocaleOutStream* los = (GuLocaleOutStream*) os;
	const uint8_t* src = utf8_src;
	const uint8_t* src_end = &src[sz];
	GuUCS* dst_end = &los->ucs_buf[GU_LOCALE_WRITER_BUF_SIZE];
	while (true) {
		gu_utf8_decode_unsafe(&src, src_end, &los->ucs_buf_curr, dst_end);
		if (src == src_end) break;
		gu_locale_writer_flush(os, exn);
		if (!gu_ok(exn)) break;
	}
	return 0;
}

GuWriter*
gu_locale_writer(GuOut* out, GuPool* pool)
{
	GuLocaleOutStream* los = gu_new_i(
		pool, GuLocaleOutStream,
		.stream.output = gu_locale_writer_output,
		.stream.flush = gu_locale_writer_flush,
		.out = out);
#ifdef GU_UCS_WCHAR
	los->ps = (mbstate_t) { 0 };
#endif
	los->ucs_buf_curr = los->ucs_buf;
	return gu_new_writer(&los->stream, pool);
}

extern inline void
gu_ucs_write(GuUCS ucs, GuWriter* wtr, GuExn* err);

extern inline void
gu_writer_flush(GuWriter* wtr, GuExn* err);

extern inline void
gu_putc(char c, GuWriter* wtr, GuExn* err);

extern inline void
gu_puts(const char* str, GuWriter* wtr, GuExn* err);

extern inline size_t
gu_utf8_write(const uint8_t* src, size_t sz, GuWriter* wtr, GuExn* err);

