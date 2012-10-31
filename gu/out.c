// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <gu/seq.h>
#include <gu/out.h>

#define GU_DEFAULT_BUFFER_SIZE 4096


GuOut
gu_init_out(GuOutStream* stream)
{
	gu_require(stream != NULL);
	GuOut out = {
		.buf_end = NULL,
		.buf_curr = 0,
		.stream = stream,
		.fini.fn = NULL
	};
	return out;
}

static bool
gu_out_is_buffering(GuOut* out)
{
	return !!out->buf_end;
}


static void
gu_out_end_buf(GuOut* out, GuExn* err)
{
	if (!gu_out_is_buffering(out)) {
		return;
	}
	GuOutStream* stream = out->stream;
	size_t curr_len = ((ptrdiff_t)out->buf_size) + out->buf_curr;
	gu_invoke(stream, end_buf, curr_len, err);
	out->buf_end = NULL;
	out->buf_size = out->buf_curr = 0;
}

static bool
gu_out_begin_buf(GuOut* out, size_t req, GuExn* err)
{
	GuOutStream* stream = out->stream;
	if (gu_out_is_buffering(out)) {
		if (out->buf_curr < 0) {
			return true;
		} else {
			gu_out_end_buf(out, err);
			if (!gu_ok(err)) {
				return false;
			}
		}
	}
	if (stream->funs->begin_buf) {
		GuSlice buf = gu_invoke(stream, begin_buf, req, err);
		size_t sz = buf.sz;
		gu_assert(sz <= PTRDIFF_MAX);
		if (sz) {
			out->buf_end = &buf.p[sz];
			out->buf_curr = -(ptrdiff_t) sz;
			out->buf_size = sz;
			return true;
		}
	}
	return false;
}



static void
gu_out_fini(GuFinalizer* self)
{
	GuOut* out = gu_container(self, GuOut, fini);
	if (gu_out_is_buffering(out)) {
		GuPool* pool = gu_local_pool();
		GuExn* err = gu_new_exn(NULL, gu_kind(type), pool);
		gu_out_end_buf(out, err);
		gu_pool_free(pool);
	}
}

GuOut*
gu_new_out(GuOutStream* stream, GuPool* pool)
{
	GuOut* out = gu_new(GuOut, pool);
	*out = gu_init_out(stream);
	out->fini.fn = gu_out_fini;
	gu_pool_finally(pool, &out->fini);
	return out;
}

extern inline bool
gu_out_try_buf_(GuOut* out, GuCSlice src);


extern inline size_t
gu_out_bytes(GuOut* out, GuCSlice src, GuExn* err);

static size_t
gu_out_output(GuOut* out, GuCSlice src, GuExn* err)
{
	gu_out_end_buf(out, err);
	if (!gu_ok(err)) {
		return 0;
	}
	return gu_invoke(out->stream, output, src, err);
}




void 
gu_out_flush(GuOut* out, GuExn* err)
{
	if (out->buf_end) {
		gu_out_end_buf(out, err);
		if (!gu_ok(err)) {
			return;
		}
	}
	gu_invoke_maybe(0, out->stream, flush, err);
}

GuSlice
gu_out_begin_span(GuOut* out, GuSlice fallback, GuExn* err)
{
	if ((!gu_out_is_buffering(out)
	     && !gu_out_begin_buf(out, fallback.sz, err))
	    || (size_t)(-out->buf_curr) < fallback.sz) {
		if (!gu_ok(err)) return gu_null_slice();
		return fallback;
	}
	return gu_slice(&out->buf_end[out->buf_curr], -out->buf_curr);
}

void
gu_out_end_span(GuOut* out, GuSlice span, GuExn* err)
{
	if (gu_out_is_buffering(out) &&
	    span.p == &out->buf_end[out->buf_curr]) {
		ptrdiff_t new_curr = (ptrdiff_t) span.sz + out->buf_curr;
		gu_require(new_curr <= 0);
		out->buf_curr = new_curr;
	} else {
		// The span is a fallback buffer, not stream buffer
		gu_out_bytes_(out, gu_slice_cslice(span), err);
	}
}

size_t
gu_out_bytes_(GuOut* restrict out, GuCSlice src, GuExn* err)
{
	if (!gu_ok(err)) {
		return 0;
	} else if (gu_out_try_buf_(out, src)) {
		return src.sz;
	}
	if (gu_out_begin_buf(out, src.sz, err)) {
		if (gu_out_try_buf_(out, src)) {
			return src.sz;
		}
	} else if (!gu_ok(err)) return 0;
	return gu_out_output(out, src, err);
}


void gu_out_u8_(GuOut* restrict out, uint8_t u, GuExn* err)
{
	if (gu_out_begin_buf(out, 1, err)) {
		if (gu_out_try_u8_(out, u)) {
			return;
		}
	} else if (!gu_ok(err)) return;
	gu_out_output(out, gu_cslice(&u, 1), err);
}


extern inline void
gu_out_u8(GuOut* restrict out, uint8_t u, GuExn* err);

extern inline void
gu_out_s8(GuOut* restrict out, int8_t i, GuExn* err);

extern inline bool
gu_out_is_buffered(GuOut* out);

extern inline bool
gu_out_try_u8_(GuOut* restrict out, uint8_t u);









typedef struct GuProxyOutStream GuProxyOutStream;

struct GuProxyOutStream {
	GuOutStream stream;
	uint8_t* buf;
	GuOut* real_out;
};


static GuSlice
gu_proxy_out_buf_begin(GuOutStream* self, size_t req, GuExn* err)
{
	GuProxyOutStream* pos = gu_container(self, GuProxyOutStream, stream);
	GuSlice ret = gu_out_begin_span(pos->real_out, gu_null_slice(), err);
	pos->buf = ret.p;
	return ret;
}

static void
gu_proxy_out_buf_end(GuOutStream* self, size_t sz, GuExn* err)
{
	GuProxyOutStream* pos =
		gu_container(self, GuProxyOutStream, stream);
	gu_out_end_span(pos->real_out, gu_slice(pos->buf, sz), err);
}

static size_t
gu_proxy_out_output(GuOutStream* self, GuCSlice src, GuExn* err)
{
	GuProxyOutStream* pos =
		gu_container(self, GuProxyOutStream, stream);
	return gu_out_bytes(pos->real_out, src, err);
}

static void
gu_proxy_out_flush(GuOutStream* self, GuExn* err)
{
	GuProxyOutStream* pos =
		gu_container(self, GuProxyOutStream, stream);
	gu_out_flush(pos->real_out, err);
}

static GuOutStreamFuns gu_proxy_out_funs = {
	.begin_buf = gu_proxy_out_buf_begin,
	.end_buf = gu_proxy_out_buf_end,
	.output = gu_proxy_out_output,
	.flush = gu_proxy_out_flush,
};


GuOutStream*
gu_out_proxy_stream(GuOut* out, GuPool* pool)
{
	return &gu_new_s(pool, GuProxyOutStream,
			 .stream.funs = &gu_proxy_out_funs,
			 .real_out = out)->stream;
}



typedef struct GuBufferedOutStream GuBufferedOutStream;

struct GuBufferedOutStream {
	GuProxyOutStream pstream;
	size_t sz;
	uint8_t buf[];
};

static GuSlice
gu_buffered_out_buf_begin(GuOutStream* self, size_t req, GuExn* err)
{
	GuBufferedOutStream* b =
		gu_container(self, GuBufferedOutStream, pstream.stream);
	return gu_slice(b->buf, b->sz);
}

static void
gu_buffered_out_buf_end(GuOutStream* self, size_t sz, GuExn* err)
{
	GuBufferedOutStream* b =
		gu_container(self, GuBufferedOutStream, pstream.stream);
	gu_require(sz <= b->sz);
	gu_out_bytes(b->pstream.real_out, gu_cslice(b->buf, sz), err);
}

static GuOutStreamFuns gu_buffered_out_funs = {
	.begin_buf = gu_buffered_out_buf_begin,
	.end_buf = gu_buffered_out_buf_end,
	.output = gu_proxy_out_output,
	.flush = gu_proxy_out_flush
};

GuOut
gu_init_buffered_out(GuOut* out, size_t sz, GuPool* pool)
{
	if (sz == 0) {
		sz = GU_DEFAULT_BUFFER_SIZE;
	}
	GuBufferedOutStream* b =
		gu_new_flex(pool, GuBufferedOutStream, buf, sz);
	b->pstream.stream.funs = &gu_buffered_out_funs;
	b->pstream.real_out = out;
	b->sz = sz;
	return gu_init_out(&b->pstream.stream);
}

GuOut*
gu_new_buffered_out(GuOut* out, size_t sz, GuPool* pool)
{
	GuOut* bout = gu_new(GuOut, pool);
	*bout = gu_init_buffered_out(out, sz, pool);
	return bout;
}

GuOut*
gu_out_buffered(GuOut* out, GuPool* pool)
{
	if (gu_out_is_buffered(out)) {
		return out;
	}
	return gu_new_buffered_out(out, 0, pool);
}


