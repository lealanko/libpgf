// Copyright 2011-2012 University of Helsinki. Released under LGPL3.

#include <gu/in.h>
#include <gu/bits.h>
#include <math.h>

GU_DEFINE_TYPE(GuEOF, abstract, _);


static bool
gu_in_is_buffering(GuIn* in)
{
	return (in->buf_end != NULL);
}

static void
gu_in_end_buffering(GuIn* in, GuExn* err)
{
	if (!gu_in_is_buffering(in)) {
		return;
	}
	size_t len = ((ptrdiff_t) in->buf_size) + in->buf_curr;
	gu_invoke_maybe(0, in->stream, end_buffer, len, err);
	in->stream_pos += len;
	in->buf_curr = 0;
	in->buf_size = 0;
	in->buf_end = NULL;
}

static bool
gu_in_begin_buffering(GuIn* in, GuExn* err)
{
	if (gu_in_is_buffering(in)) {
		if (in->buf_curr < 0) {
			return true;
		} else {
			gu_in_end_buffering(in, err);
			if (!gu_ok(err)) return false;
		}
	}
	if (!in->stream->funs->begin_buffer) {
		return false;
	}
	GuCSlice new_buf = gu_invoke(in->stream, begin_buffer, err);
	size_t sz = new_buf.sz;
	if (gu_ok(err) && sz > 0) {
		in->buf_end = &new_buf.p[sz];
		in->buf_curr = -(ptrdiff_t) sz;
		in->buf_size = sz;
		return true;
	}
	return false;
}

static size_t
gu_in_input(GuIn* in, GuSlice dst, GuExn* err)
{
	if (dst.sz == 0) {
		return 0;
	}
	gu_in_end_buffering(in, err);
	if (!gu_ok(err)) {
		return 0;
	}
	size_t len = gu_invoke_maybe(0, in->stream, input, dst, err);
	in->stream_pos += len;
	return len;
}

size_t
gu_in_some(GuIn* in, GuSlice dst, size_t min_sz, GuExn* err)
{
	gu_require(min_sz <= dst.sz);
	gu_require(dst.sz <= PTRDIFF_MAX);
	size_t got = 0;
	if (gu_in_begin_buffering(in, err)) {
		got = GU_MIN(dst.sz, (size_t)(-in->buf_curr));
		memcpy(dst.p, &in->buf_end[in->buf_curr], got);
		in->buf_curr += got;
	} else if (!gu_ok(err)) return 0;
	while (got < min_sz) {
		GuSlice req = { &dst.p[got], dst.sz - got };
		size_t igot = gu_in_input(in, req, err);
		if (igot == 0) break; // EOF
		got += igot;
	}
	return got;
}

void
gu_in_bytes_(GuIn* in, GuSlice dst, GuExn* err)
{
	size_t got = gu_in_some(in, dst, dst.sz, err);
	if (!gu_ok(err)) return;
	if (got < dst.sz) {
		gu_raise(err, GuEOF);
	}
}

GuCSlice
gu_in_begin_span(GuIn* in, GuSlice req, GuExn* err)
{
	GuCSlice ret;
	if (!gu_in_begin_buffering(in, err)) {
		ret.p = req.p;
		ret.sz = gu_in_input(in, req, err);
	} else {
		ret.p = &in->buf_end[in->buf_curr];
		ret.sz = (size_t) -in->buf_curr;
	}
	return ret;
}

void
gu_in_end_span(GuIn* in, size_t consumed)
{
	if (!gu_in_is_buffering(in)) {
		// we were using the fallback buffer
		// TODO: assert that consumed size equals fallback buffer size
		return;
	}
	gu_require(consumed <= (size_t) -in->buf_curr);
	in->buf_curr += (ptrdiff_t) consumed;
}

uint8_t 
gu_in_u8_(GuIn* in, GuExn* err)
{
	if (gu_in_begin_buffering(in, err) && in->buf_curr < 0) {
		return in->buf_end[in->buf_curr++];
	}
	uint8_t u = 0;
	GuSlice req = { &u, 1 };
	size_t r = gu_in_input(in, req, err);
	if (r < 1) {
		if (gu_ok(err)) {
			gu_raise(err, GuEOF);
		}
		return 0;
	}
	return u;
}

static uint64_t
gu_in_be(GuIn* in, GuExn* err, int n)
{
	uint8_t buf[8];
	GuSlice req = { buf, n };
	gu_in_bytes(in, req, err);
	uint64_t u = 0;
	for (int i = 0; i < n; i++) {
		u = u << 8 | buf[i];
	}
	return u;
}

static uint64_t
gu_in_le(GuIn* in, GuExn* err, int n)
{
	uint8_t buf[8];
	GuSlice req = { buf, n };
	gu_in_bytes(in, req, err);
	uint64_t u = 0;
	for (int i = 0; i < n; i++) {
		u = u << 8 | buf[i];
	}
	return u;
}

int8_t 
gu_in_s8(GuIn* in, GuExn* err)
{
	return gu_decode_2c8(gu_in_u8(in, err), err);
}


uint16_t
gu_in_u16le(GuIn* in, GuExn* err)
{
	return gu_in_le(in, err, 2);
}

int16_t 
gu_in_s16le(GuIn* in, GuExn* err)
{
	return gu_decode_2c16(gu_in_u16le(in, err), err);
}

uint16_t
gu_in_u16be(GuIn* in, GuExn* err)
{
	return gu_in_be(in, err, 2);
}

int16_t 
gu_in_s16be(GuIn* in, GuExn* err)
{
	return gu_decode_2c16(gu_in_u16be(in, err), err);
}


uint32_t
gu_in_u32le(GuIn* in, GuExn* err)
{
	return gu_in_le(in, err, 4);
}

int32_t 
gu_in_s32le(GuIn* in, GuExn* err)
{
	return gu_decode_2c32(gu_in_u32le(in, err), err);
}

uint32_t
gu_in_u32be(GuIn* in, GuExn* err)
{
	return gu_in_be(in, err, 4);
}

int32_t 
gu_in_s32be(GuIn* in, GuExn* err)
{
	return gu_decode_2c32(gu_in_u32be(in, err), err);
}


uint64_t
gu_in_u64le(GuIn* in, GuExn* err)
{
	return gu_in_le(in, err, 8);
}

int64_t 
gu_in_s64le(GuIn* in, GuExn* err)
{
	return gu_decode_2c64(gu_in_u64le(in, err), err);
}

uint64_t
gu_in_u64be(GuIn* in, GuExn* err)
{
	return gu_in_be(in, err, 8);
}

int64_t 
gu_in_s64be(GuIn* in, GuExn* err)
{
	return gu_decode_2c64(gu_in_u64be(in, err), err);
}

double
gu_in_f64le(GuIn* in, GuExn* err)
{
	return gu_decode_double(gu_in_u64le(in, err));
}

double
gu_in_f64be(GuIn* in, GuExn* err)
{
	return gu_decode_double(gu_in_u64le(in, err));
}


static void
gu_in_fini(GuFinalizer* fin)
{
	GuIn* in = gu_container(fin, GuIn, fini);
	GuPool* pool = gu_local_pool();
	GuExn* err = gu_exn(NULL, type, pool);
	gu_in_end_buffering(in, err);
	gu_pool_free(pool);
}

GuIn
gu_init_in(GuInStream* stream)
{
	return (GuIn) {
		.buf_end = NULL,
		.buf_curr = 0,
		.buf_size = 0,
		.stream = stream,
		.stream_pos = 0,	
		.fini.fn = gu_in_fini
	};
}

GuIn*
gu_new_in(GuInStream* stream, GuPool* pool)
{
	GuIn* in = gu_new(GuIn, pool);
	*in = gu_init_in(stream);
	return in;
}

size_t
gu_in_tell(GuIn* in)
{
	return in->stream_pos + in->buf_size + in->buf_curr;
}


typedef struct GuProxyInStream GuProxyInStream;

struct GuProxyInStream {
	GuInStream stream;
	GuIn* real_in;
};

static GuCSlice
gu_proxy_in_begin_buffer(GuInStream* self, GuExn* err)
{
	GuProxyInStream* pis = gu_container(self, GuProxyInStream, stream);
	GuSlice req = { NULL, 0 };
	return gu_in_begin_span(pis->real_in, req, err);
}

static void
gu_proxy_in_end_buffer(GuInStream* self, size_t sz, GuExn* err)
{
	GuProxyInStream* pis = gu_container(self, GuProxyInStream, stream);
	gu_in_end_span(pis->real_in, sz);
}

static size_t
gu_proxy_in_input(GuInStream* self, GuSlice dst, GuExn* err)
{
	GuProxyInStream* pis = gu_container(self, GuProxyInStream, stream);
	return gu_in_some(pis->real_in, dst, 1, err);
}

static GuInStreamFuns gu_proxy_in_funs = {
	.begin_buffer = gu_proxy_in_begin_buffer,
	.end_buffer = gu_proxy_in_end_buffer,
	.input = gu_proxy_in_input
};

GuInStream*
gu_in_proxy_stream(GuIn* in, GuPool* pool)
{
	return &gu_new_s(
		pool, GuProxyInStream,
		.stream.funs = &gu_proxy_in_funs,
		.real_in = in)->stream;
}

enum {
	GU_BUFFERED_IN_BUF_SIZE = 4096
};

typedef struct GuBufferedInStream GuBufferedInStream;

struct GuBufferedInStream {
	GuInStream stream;
	size_t alloc;
	size_t have;
	size_t curr;
	GuIn* in;
	uint8_t buf[];
};

static GuCSlice
gu_buffered_in_begin_buffer(GuInStream* self, GuExn* err)
{
	GuBufferedInStream* bis = 
		gu_container(self, GuBufferedInStream, stream);
	if (bis->curr == bis->have) {
		bis->curr = 0;
		GuSlice req = { bis->buf, bis->alloc };
		bis->have = gu_in_some(bis->in, req, 1, err);
		if (!gu_ok(err)) return gu_null_cslice();
	}
	return gu_cslice(&bis->buf[bis->curr], bis->have - bis->curr);
}

static void
gu_buffered_in_end_buffer(GuInStream* self, size_t consumed, GuExn* err)
{
	GuBufferedInStream* bis = 
		gu_container(self, GuBufferedInStream, stream);
	gu_require(consumed <= bis->have - bis->curr);
	bis->curr += consumed;
}

static size_t
gu_buffered_in_input(GuInStream* self, GuSlice dst, GuExn* err)
{
	GuBufferedInStream* bis = 
		gu_container(self, GuBufferedInStream, stream);
	return gu_in_some(bis->in, dst, 1, err);
}

static GuInStreamFuns gu_buffered_in_funs = {
	.begin_buffer = gu_buffered_in_begin_buffer,
	.end_buffer = gu_buffered_in_end_buffer,
	.input = gu_buffered_in_input
};

GuIn*
gu_new_buffered_in(GuIn* in, size_t buf_sz, GuPool* pool)
{
	GuBufferedInStream* bis = gu_new_flex(pool, GuBufferedInStream,
					      buf, buf_sz);
	bis->stream.funs = &gu_buffered_in_funs;
	bis->have = bis->curr = 0;
	bis->alloc = buf_sz;
	bis->in = in;
	return gu_new_in(&bis->stream, pool);
}

typedef struct GuDataIn GuDataIn;

struct GuDataIn {
	GuInStream stream;
	GuCSlice data;
};

static GuCSlice
gu_data_in_begin_buffer(GuInStream* self, GuExn* err)
{
	GuDataIn* di = gu_container(self, GuDataIn, stream);
	return di->data;
}

static void
gu_data_in_end_buffer(GuInStream* self, size_t consumed, GuExn* err)
{
	GuDataIn* di = gu_container(self, GuDataIn, stream);
	di->data.p += consumed;
	di->data.sz -= consumed;
}

static GuInStreamFuns
gu_data_in_funs = {
	.begin_buffer = gu_data_in_begin_buffer,
	.end_buffer = gu_data_in_end_buffer
};

GuIn*
gu_data_in(GuCSlice data, GuPool* pool)
{
	GuDataIn* di = gu_new_s(pool, GuDataIn,
				.stream.funs = &gu_data_in_funs,
				.data = data);
	return gu_new_in(&di->stream, pool);
}

extern inline uint8_t 
gu_in_u8(GuIn* restrict in, GuExn* err);

extern inline void
gu_in_bytes(GuIn* in, GuSlice buf, GuExn* err);

extern inline int
gu_in_peek_u8(GuIn* restrict in);

extern inline void
gu_in_consume(GuIn* restrict in, size_t sz);
