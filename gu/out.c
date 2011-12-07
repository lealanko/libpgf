#include <gu/seq.h>
#include <gu/out.h>


#define GU_OUT_BUF_THRESHOLD 1024

extern inline bool
gu_out_try_buf_(GuOut* out, const uint8_t* src, size_t len);


extern inline size_t
gu_out_bytes(GuOut* out, const uint8_t* buf, size_t len, GuError* err);

static size_t
gu_out_output(GuOut* out, const uint8_t* src, size_t len, GuError* err)
{
	return out->output(out, src, len, err);
}


static bool
gu_out_begin_buf(GuOut* out)
{
	GuOutBuffer* buffer = out->buffer;
	gu_assert(!out->buf_start);
	if (buffer) {
		size_t sz = 0;
		uint8_t* buf = buffer->begin(buffer, &sz);
		gu_assert(sz <= PTRDIFF_MAX);
		if (buf) {
			out->buf_start = buf;
			out->buf_end = &buf[sz];
			out->buf_curr = -(ptrdiff_t) sz;
			return true;
		}
	}
	return false;
}


static void 
gu_out_end_buf(GuOut* out, GuError* err)
{
	if (out->buf_start) {
		size_t curr_len = 
			(out->buf_end - out->buf_start) + out->buf_curr;
		out->buffer->end(out->buffer, curr_len, err);
		out->buf_start = out->buf_end = NULL;
		out->buf_curr = 0;
	}
}


void
gu_out_set_buffer(GuOut* out, GuOutBuffer* buffer, GuError* err)
{
	gu_out_end_buf(out, err);
	out->buffer = buffer;
}

void
gu_out_enable_buffering(GuOut* out, size_t buf_sz, GuPool* pool)
{
	if (gu_out_is_buffered(out)) {
		return;
	}
	GuOutBuffer* buffer = gu_out_make_buffer(out, buf_sz, pool);
	gu_out_set_buffer(out, buffer, NULL);
}

uint8_t*
gu_out_begin_span(GuOut* out, size_t* sz_out)
{
	if (!out->buf_end) {
		return NULL;
	}
	*sz_out = -out->buf_curr;
	return &out->buf_end[out->buf_curr];
}

void
gu_out_end_span(GuOut* out, size_t sz)
{
	ptrdiff_t new_curr = (ptrdiff_t) sz + out->buf_curr;
	gu_require(new_curr <= 0);
	out->buf_curr = new_curr;
}

typedef struct GuSimpleOutBuffer GuSimpleOutBuffer;

struct GuSimpleOutBuffer {
	GuOutBuffer buffer;
	GuOut* out;
	size_t sz;
	uint8_t data[];
};

uint8_t*
gu_simple_out_buf_begin(GuOutBuffer* self, size_t* sz_out)
{
	GuSimpleOutBuffer* b = (GuSimpleOutBuffer*) self;
	*sz_out = b->sz;
	return b->data;
}

void
gu_simple_out_buf_end(GuOutBuffer* self, size_t sz, GuError* err)
{
	GuSimpleOutBuffer* b = (GuSimpleOutBuffer*) self;
	gu_require(sz <= b->sz);
	gu_out_output(b->out, b->data, sz, err);
}

GuOutBuffer*
gu_out_make_buffer(GuOut* out, size_t sz, GuPool* pool)
{
	GuSimpleOutBuffer* b = gu_flex_new(pool, GuSimpleOutBuffer, data, sz);
	b->buffer.begin = gu_simple_out_buf_begin;
	b->buffer.end = gu_simple_out_buf_end;
	b->out = out;
	b->sz = sz;
	return (GuOutBuffer*) b;
}

size_t
gu_out_bytes_(GuOut* restrict out, const uint8_t* restrict src, size_t len, 
	      GuError* err)
{
	if (!gu_ok(err)) {
		return 0;
	} else if (gu_out_try_buf_(out, src, len)) {
		return len;
	}
	gu_out_end_buf(out, err);
	if (!gu_ok(err)) {
		return 0;
	} else if (len <= GU_OUT_BUF_THRESHOLD && gu_out_begin_buf(out)) {
		if (gu_out_try_buf_(out, src, len)) {
			return len;
		} else {
			gu_out_end_buf(out, err);
		}
	}
	return gu_out_output(out, src, len, err);
}

void
gu_out_flush(GuOut* out, GuError* err)
{
	gu_out_end_buf(out, err);
	if (gu_ok(err) && out->flush) {
		out->flush(out, err);
	}
}


extern inline void
gu_out_u8(GuOut* restrict out, uint8_t u, GuError* err);

extern inline void
gu_out_s8(GuOut* restrict out, int8_t i, GuError* err);

extern inline bool
gu_out_is_buffered(GuOut* out);

extern inline bool
gu_out_try_u8_(GuOut* restrict out, uint8_t u);
