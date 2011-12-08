#include <gu/write.h>


size_t
gu_utf32_write(const GuUCS* src, size_t len, GuWriter* wtr, GuError* err)
{
	return gu_utf32_out_utf8(src, len, &wtr->out_, err);
}


void
gu_vprintf(const char* fmt, va_list args, GuWriter* wtr, GuError* err)
{
	GuPool* tmp_pool = gu_local_pool();
	char* str = gu_vasprintf(fmt, args, tmp_pool);
	gu_puts(str, wtr, err);
	gu_pool_free(tmp_pool);
}

void
gu_printf(GuWriter* wtr, GuError* err, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	gu_vprintf(fmt, args, wtr, err);
	va_end(args);
}

void
gu_out_writer_flush(GuOut* out, GuError* err)
{
	GuOutWriter* owtr = gu_container(out, GuOutWriter, wtr.out_);
	gu_out_flush(owtr->out, err);
}


typedef struct GuUTF8Writer GuUTF8Writer;

struct GuUTF8Writer {
	GuOutWriter owtr;
	GuOutBuffer buffer;
};

size_t
gu_utf8_writer_output(GuOut* out, const uint8_t* src, size_t sz, 
		      GuError* err)
{
	GuUTF8Writer* uwtr = gu_container(out, GuUTF8Writer, owtr.wtr.out_);
	return gu_out_bytes(uwtr->owtr.out, src, sz, err);
}

uint8_t*
gu_utf8_writer_buf_begin(GuOutBuffer* buffer, size_t* sz_out)
{
	GuUTF8Writer* uwtr = gu_container(buffer, GuUTF8Writer, buffer);
	return gu_out_begin_span(uwtr->owtr.out, sz_out);
}

void
gu_utf8_writer_buf_end(GuOutBuffer* buffer, size_t sz, GuError* err)
{
	(void) err;
	GuUTF8Writer* uwtr = gu_container(buffer, GuUTF8Writer, buffer);
	gu_out_end_span(uwtr->owtr.out, sz);
}


GuWriter*
gu_make_utf8_writer(GuOut* utf8_out, GuPool* pool)
{
	GuUTF8Writer* uwtr = gu_new_i(
		pool, GuUTF8Writer,
		.owtr.wtr.out_.output = gu_utf8_writer_output,
		.owtr.wtr.out_.flush = gu_out_writer_flush,
		.owtr.out = utf8_out,
		.buffer.begin = gu_utf8_writer_buf_begin,
		.buffer.end = gu_utf8_writer_buf_end);
	uwtr->owtr.wtr.out_.buffer = &uwtr->buffer;
	return &uwtr->owtr.wtr;
}


#if 0
#ifdef GU_UCS_WCHAR
#include <stdlib.h>
#include <wchar.h>
static const mbstate_t gu_init_mbstate; // implicitly initialized to zero
#endif

typedef struct GuLocaleWriter GuLocaleWriter;

struct GuLocaleWriter {
	GuOutWriter owtr;
#ifdef GU_UCS_WCHAR
	mbstate_t ps;
	size_t mb_cur_max;
#endif
};

size_t
gu_locale_writer_write(GuWriter* wtr, const uint8_t* utf8_src, size_t sz, 
		       GuError* err)
{
	GuLocaleWriter* lwtr = (GuLocaleWriter*) wtr;
	size_t done = 0;
	static const size_t bufsize = 256;
#ifdef GU_UCS_WCHAR
	size_t margin = lwtr->mb_cur_max;
#else
	size_t margin = 1;
#endif
	GuOut* out = lwtr->owtr.out;
	if (gu_out_is_buffered(out)) {
		while (done < sz) {
			size_t dst_sz;
			uint8_t* dst = gu_out_begin_span(out, &dst_sz);
			if (!dst) {
				break;
			}
			if (dst_sz <= margin) {
				gu_out_end_span(out, 0);
				break;
			}
			size_t end = dst_sz - margin;
			const uint8_t* 
			size_t n = done;
			while (n < sz && dst_i <= end) {
#ifdef GU_UCS_WCHAR
				GuUCS ucs = gu_
				wchar_t wc = src[n];
				size_t nb = wcrtomb((char*) p, wc, &lwtr->ps);
#else
			*p = (uint8_t) gu_ucs_char(buf[n], err);
			size_t nb = 1;
			if (!gu_ok(err)) {
				gu_error_clear(err);
				nb = (size_t) -1;
			}
#endif
			if (nb == (size_t) -1) {
				*p++ = (uint8_t) '?';
			} else {
				p += nb;
			}
				
			}
			for (

		}



	}

	uint8_t cbuf[256];
	while (done < size && gu_ok(err)) {
		uint8_t* p = cbuf;
		uint8_t* edge = &cbuf[bufsize - margin];
		size_t n;
		for  (n = done; p <= edge && n < size; n++) {
#ifdef GU_UCS_WCHAR
			wchar_t wc = buf[n];
			size_t nb = wcrtomb((char*) p, wc, &lwtr->ps);
#else
			*p = (uint8_t) gu_ucs_char(buf[n], err);
			size_t nb = 1;
			if (!gu_ok(err)) {
				gu_error_clear(err);
				nb = (size_t) -1;
			}
#endif
			if (nb == (size_t) -1) {
				*p++ = (uint8_t) '?';
			} else {
				p += nb;
			}
		}
		gu_out_bytes(lwtr->owtr.out, cbuf, p - cbuf, err);
		if (gu_ok(err)) {
			done = n;
		}
	}
	return done;
}

GuWriter*
gu_locale_writer(GuOut* out, GuPool* pool)
{
	GuLocaleWriter* lwtr = gu_new_s(
		pool, GuLocaleWriter,
		.wtr.out.output = gu_locale_writer_output,
		.wtr.out.flush = gu_locale_writer_flush,
		.out = out);
#ifdef GU_UCS_WCHAR
	lwtr->ps = gu_init_mbstate;
	lwtr->mb_cur_max = MB_CUR_MAX;
#endif
	return (GuWriter*) lwtr;
}

#endif

extern inline void
gu_ucs_write(GuUCS ucs, GuWriter* wtr, GuError* err);

extern inline void
gu_writer_flush(GuWriter* wtr, GuError* err);

extern inline void
gu_putc(char c, GuWriter* wtr, GuError* err);

extern inline void
gu_puts(const char* str, GuWriter* wtr, GuError* err);

extern inline size_t
gu_utf8_write(const uint8_t* src, size_t sz, GuWriter* wtr, GuError* err);

