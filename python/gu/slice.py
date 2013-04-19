from .core import *

class CSlice(CStructure):
    p = Field(c_uint8_p)
    sz = Field(c_size_t)
    def bytes(self):
        return string_at(self.p, self.sz)
    def __getitem__(self, idx):
        if idx < 0 or idx >= self.sz:
            raise ValueError
        return self.p[idx]
    def __len__(self):
        return self.sz
    def _array(self):
        return cast(self.p, Address)[c_byte * self.sz]
    
    @classmethod
    def copy_buffer(cls, buf):
        arr = create_string_buffer(buf, len(buf))
        return cls.of_buffer(arr)

    @classmethod
    def of_buffer(cls, buf):
        sz = len(buf)
        arr = (c_uint8 * sz).from_buffer(buf)
        return cls(p=cast(arr, c_uint8_p), sz=sz)

    def __iter__(self):
        for i in range(self.sz):
            yield self.p[i]

class Slice(CSlice):
    def __setitem__(self, idx, value):
        if idx < 0 or ifx >= self.sz:
            raise ValueError
        self.p[idx] = value
    def array(self):
        return self._array()

class BytesCSlice(util.instance(Spec)):
    c_type = CSlice

    def to_py(c_val):
        return c_val.bytes()
    
    def as_c(b):
        yield CSlice(p=cast(b, c_uint8_p), sz=len(b))

class _CSliceSpec(util.instance(ProxySpec)):
    sot = cspec(CSlice)
    def wrap(c_val):
        # XXX: return read-only memoryview, once possible
        return c_val.bytes()
    def unwrap(buf):
        # XXX: accept read-only buffer, once possible
        return buf if isinstance(buf, CSlice) else CSlice.copy_buffer(buf)

CSlice.default_spec = _CSliceSpec
    
class _SliceSpec(util.instance(ProxySpec)):
    sot = cspec(Slice)
    def wrap(c_val):
        return c_val.array()
    def unwrap(buf):
        return buf if isutil.instance(buf, Slice) else Slice.of_buffer(buf)

Slice.default_spec = _SliceSpec

