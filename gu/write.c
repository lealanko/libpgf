#include <gu/write.h>


size_t
gu_write(GuWriter* wtr, const GuUCS* buf, size_t size, GuError* err)
{
	if (!gu_ok(err)) {
		return 0;
	}
	return wtr->write(wtr, buf, size, err);
}

void
gu_writer_flush(GuWriter* wtr, GuError* err)
{
	if (wtr->flush) {
		wtr->flush(wtr, err);
	}
}

void
gu_putc(char c, GuWriter* wtr, GuError* err)
{
	if (!gu_char_is_valid(c)) {
		gu_raise(err, GuUCSError);
		return;
	}
	GuUCS ucs = gu_char_ucs(c);
	gu_write(wtr, &ucs, 1, err);
}

void
gu_puts(const char* str, GuWriter* wtr, GuError* err)
{
	size_t done = 0;
	size_t size = strlen(str);
	while (done < size && gu_ok(err)) {
		GuUCS buf[256];
		size_t len = GU_MIN(256, size - done);
		size_t n = gu_str_to_ucs(&str[done], len, buf, err);
		gu_error_block(err);
		gu_write(wtr, buf, n, err);
		gu_error_unblock(err);
		done += n;
	}
}

void
gu_vprintf(const char* fmt, va_list args, GuWriter* wtr, GuError* err)
{
	GuPool* tmp_pool = gu_pool_new();
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
gu_out_writer_flush(GuWriter* wtr, GuError* err)
{
	GuOutWriter* owtr = (GuOutWriter*) wtr;
	gu_out_flush(owtr->out, err);
}



typedef GuOutWriter GuCharWriter;

size_t
gu_char_writer_write(GuWriter* wtr, 
		     const GuUCS* buf, size_t size, GuError* err)
{
	GuCharWriter* cwtr = (GuCharWriter*) wtr;
	size_t done = 0;
	while (done < size && gu_ok(err)) {
		uint8_t cbuf[256];
		size_t len = GU_MIN(256, size - done);
		size_t n = gu_ucs_to_str(&buf[done], len, (char*) cbuf, err);
		gu_error_block(err);
		size_t wrote = gu_out_bytes(cwtr->out, cbuf, n, err);
		done += wrote;
		gu_error_unblock(err);
	}
	return done;
}

GuWriter*
gu_char_writer(GuOut* out, GuPool* pool)
{
	return (GuWriter*) gu_new_s(
		pool, GuCharWriter,
		.wtr = { .write = gu_char_writer_write,
			 .flush = gu_out_writer_flush },
		.out = out);
}

typedef struct GuBufferedWriter GuBufferedWriter;

struct GuBufferedWriter {
	GuWriter wtr;
	GuWriter* real_wtr;
	GuUCS* buf;
	size_t idx;
	size_t size;
};

size_t
gu_buffered_writer_write(GuWriter* wtr, 
			 const GuUCS* buf, size_t size, GuError* err)
{
	GuBufferedWriter* bwtr = (GuBufferedWriter*) wtr;
	size_t bsize = bwtr->size;
	size_t bidx = bwtr->idx;
	GuUCS* bbuf = bwtr->buf;
	size_t done = 0;

	if (size == 1) {
		bbuf[bidx++] = buf[0];
	} else if (size > bsize) {
		if (bidx > 0) {
			gu_write(bwtr->real_wtr, bbuf, bidx, err);
		}
		if (!gu_ok(err)) {
			bwtr->idx = 0;
			return 0;
		}
		return gu_write(bwtr->real_wtr, buf, size, err);
	} else {
		size_t copy = GU_MIN(size, bsize - bidx);
		memcpy(&bbuf[bidx], buf, copy * sizeof(GuUCS));
		bidx += copy;
		done = copy;
	}
	if (bidx == bsize) {
		size_t wrote = gu_write(bwtr->real_wtr, bbuf, bsize, err);
		if (wrote < bsize) {
			size_t ret = wrote > bwtr->idx ? wrote - bwtr->idx : 0;
			bwtr->idx = 0;
			return ret;
		}
		bidx = 0;
		if (done < size) {
			memcpy(bbuf, &buf[done],
			       (size - done) * sizeof(GuUCS));
			bidx += (size - done);
		}
	}
	bwtr->idx = bidx;
	return size;
}

void
gu_buffered_writer_flush(GuWriter* wtr, GuError* err)
{
	GuBufferedWriter* bwtr = (GuBufferedWriter*) wtr;
	gu_write(bwtr->real_wtr, bwtr->buf, bwtr->idx, err);
	gu_writer_flush(bwtr->real_wtr, err);
	bwtr->idx = 0;
}

GuWriter*
gu_buffered_writer(GuWriter* real_wtr, size_t buf_size, GuPool* pool)
{
	return (GuWriter*) gu_new_i(
		pool, GuBufferedWriter,
		.wtr = { .write = gu_buffered_writer_write,
			 .flush = gu_buffered_writer_flush },
		.real_wtr = real_wtr,
		.buf = gu_new_n(pool, GuUCS, buf_size),
		.size = buf_size,
		.idx = 0);
}

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
gu_locale_writer_write(GuWriter* wtr, 
		       const GuUCS* buf, size_t size, GuError* err)
{
	GuLocaleWriter* lwtr = (GuLocaleWriter*) wtr;
	size_t done = 0;
	static const size_t bufsize = 256;
#ifdef GU_UCS_WCHAR
	size_t margin = lwtr->mb_cur_max;
#else
	size_t margin = 1;
#endif
	uint8_t cbuf[256];
	while (done < size && gu_ok(err)) {
		char* p = (char*) cbuf;
		char* edge = (char*) &cbuf[bufsize - margin];
		size_t n;
		for  (n = done; p <= edge && n < size; n++) {
#ifdef GU_UCS_WCHAR
			wchar_t wc = buf[n];
			size_t nb = wcrtomb(p, wc, &lwtr->ps);
#else
			*p = gu_ucs_char(buf[n], err);
			size_t nb = 1;
			if (!gu_ok(err)) {
				gu_error_clear(err);
				nb = (size_t) -1;
			}
#endif
			if (nb == (size_t) -1) {
				*p++ = '?';
			} else {
				p += nb;
			}
		}
		gu_out_bytes(lwtr->owtr.out, cbuf, n, err);
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
		.owtr = {
			.wtr = { .write = gu_locale_writer_write,
				 .flush = gu_out_writer_flush },
   		        .out = out });
#ifdef GU_UCS_WCHAR
	lwtr->ps = gu_init_mbstate;
	lwtr->mb_cur_max = MB_CUR_MAX;
#endif
	return (GuWriter*) lwtr;
}

