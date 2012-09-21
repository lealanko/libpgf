from codecs import utf_8_encode, utf_8_decode
from io import RawIOBase, BufferedIOBase, TextIOBase, UnsupportedOperation
from util import *
from ffi import *

gu = Library('libgu.so.0', "gu_")

class Abstract(Structure, CBase):
    @classproperty
    def default_spec(self):
        return cref(self)


class Pool(Abstract):
    alive = True
    own = False

    def __new__(cls):
        p = cls.new()
        p.own = True
        return p

    def _close(self):
        if self.alive:
            self._free()
            self.alive = False

    def __del__(self):
        if self.own:
            self._close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self._close()

Pool.new = gu.new_pool.static(Pool)
Pool._free = gu.pool_free(None, Pool)


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

class Slice(CSlice):
    def __setitem__(self, idx, value):
        if idx < 0 or ifx >= self.sz:
            raise ValueError
        self.p[idx] = value
    def array(self):
        return self._array()

class BytesCSlice(instance(Spec)):
    c_type = CSlice

    def to_py(c_val):
        return c_val.bytes()
    
    def as_c(b):
        yield CSlice(p=cast(b, c_uint8_p), sz=len(b))

class _CSliceSpec(instance(ProxySpec)):
    sot = cspec(CSlice)
    def wrap(c_val):
        # XXX: return read-only memoryview, once possible
        return c_val.bytes()
    def unwrap(buf):
        # XXX: accept read-only buffer, once possible
        return buf if isinstance(buf, CSlice) else CSlice.copy_buffer(buf)

CSlice.default_spec = _CSliceSpec
    
class _SliceSpec(instance(ProxySpec)):
    sot = cspec(Slice)
    def wrap(c_val):
        return c_val.array()
    def unwrap(buf):
        return buf if isinstance(buf, Slice) else Slice.of_buffer(buf)

Slice.default_spec = _SliceSpec

class _Pool_o(instance(CSpec)):
    c_type = IPOINTER(Pool)
    is_dep = True

    def as_c(p):
        if p is None:
            p = Pool()
        yield byref(p)

    def to_py(c):
        return c[0]

Pool.Out = dep(default(Pool, lambda: Pool()))
    



##
## Types
##


class Kind(CStructure, delay=True):
    @classmethod
    def bind(cls, c_type, name):
        t = gu['gu_type__' + name][cls]
        t.c_type = c_type
        _gu_types[c_type] = t

    @classproperty
    @memo
    def spec(cls):
        return type_spec(cref(cls))

    # TODO: autocreate c_type for GuTypes that haven't been explicitly
    # bound
        

Kind.super = Field(cref(Kind))
Kind.init()

_gu_types = dict()

def gu_type(t):
    if isinstance(t, Kind):
        return t
    elif isinstance(t, CDataType):
        return _gu_types[t]
    elif isinstance(t, Spec):
        return _gu_types[t.c_type]
    else:
        raise TypeError

class TypeSpec(ProxySpec):
    def unwrap(self, t):
        try:
            return gu_type(t)
        except KeyError:
            raise KeyError('No GuType known for type', t)

    def wrap(self, c):
        if c is None:
            return None
        try:
            return c.c_type
        except AttributeError:
            raise KeyError('No C type known for GuType', c)

def type_spec(sot):
    return TypeSpec(sot=sot)


class TypeRefSpec(RefSpec):
    def find_ref(self, c):
        return get_ref(c[0].super.c_type, c)

def type_ref(sot):
    return TypeRefSpec(sot=sot)

class Type(Kind):
    @classproperty
    @memo
    def spec(cls):
        return type_spec(type_ref(cls))

Kind.bind(Type, 'type')
Type.has_kind = gu.type_has_kind(c_bool, Type.spec, Kind.spec)

class TypeRepr(Type):
    size = Field(c_uint16)
    align = Field(c_uint16)

Kind.bind(TypeRepr, 'repr')

class PrimType(TypeRepr):
    name = Field(c_char_p)

Kind.bind(PrimType, 'primitive')


PrimType.bind(None, 'void')
PrimType.bind(c_int, 'int')

class AbstractType(Type):
    pass

Kind.bind(AbstractType, 'abstract')

class StructRepr(TypeRepr):
    name = Field(c_char_p)
    # members = Field(SList(Member))

Kind.bind(StructRepr, 'struct')


##
## Exceptions
##

class Exn(Abstract):
    def __new__(cls, exn=None, kind=None, pool=None):
        if kind is None:
            kind = Type
        return cls.new(exn, kind, pool)

Exn.new = gu.new_exn.static(Exn, Exn, Kind.spec, Pool.Out)
Exn.caught = gu.exn_caught(Type.spec, Exn)
Exn.caught_data = gu.exn_caught_data(Address, Exn)

# Temporary wrapper until we integrate libgu exception types into the
# python exception hierarchy
class GuException(Exception):
    pass

class _CurrentExnSpec(instance(ProxySpec)):
    sot = Exn
    def as_c(e):
        if e is None:
            e = Exn()
        for c in Exn.Out.spec.as_c(e):
            yield c
        t = e.caught()
        if t:
            addr = e.caught_data()
            s = spec(t)
            val = None
            if addr:
                val = s.from_addr(addr)
                val.add_dep(e) # More precisely: the pool of e
            raise GuException(s.c_type, val)

    def as_py(c, ctx):
        try:
            yield []
        except Exception as e:
            import traceback
            traceback.print_exc()
            # TODO: convert e to GuException
            pass

Exn.Out = _CurrentExnSpec

class ExnData(CStructure):
    pool = Field(Pool)
    data = Field(Address)

    


#
#
#   

class OpaqueType(CStructureType):
    def __call__(cls, *args):
        return cls.__new__(cls, *args)

class Opaque(CStructure, metaclass=OpaqueType):
    w_ = Field(c_uintptr)


class SeqType(CStructType):
    def __new__(cls, name, parents, d):
        d._elemptr_type = POINTER(d.elemspec.c_type)
        return CStructType.__new__(cls, name, parents, d)



#
# Seq
#

class Seq(Opaque):
    def _arr(self):
        arrtype = self.elemspec.c_type * self.length()
        return cast(self._data(), POINTER(arrtype))[0]

    def _elemdata(self):
        return cast(self._data(), POINTER(self.elemspec.c_type))

    def __getitem__(self, idx):
        return self.elemspec.to_py(self._elemdata()[idx])

    def __setitem__(self, idx, val):
        self._elemdata()[idx] = self.elemspec.as_c(val)

Seq._data = gu.seq_data(c_void_p, Seq)
Seq.length = gu.seq_length(c_size_t, Seq)

class Bytes(Seq):
    elemspec = cspec(c_byte)
    def bytes(self):
        return bytes(self._arr())

#
# String
#

class String(Opaque):
    def __new__(cls, s, pool = None):
        b, _ = utf_8_encode(s)
        return cls.from_utf8(b, pool)

    def __str__(self):
        slc = self.utf8()
        s, _ = utf_8_decode(slc)
        return s

    def __eq__(self, other):
        return self.eq(other)

String.from_utf8 = gu.utf8_string.static(cspec(String), BytesCSlice, Pool.Out)
String.utf8 = gu.string_utf8(Slice, cspec(String), Pool.Out)

class _StringSpec(instance(ProxySpec)):
    sot = cspec(String)
    def unwrap(s):
        return String(s) if isinstance(s, str) else s
    def wrap(s):
        return str(s)

String.default_spec = _StringSpec
String.eq = gu.string_eq(c_bool, String, String)

#
# In
#

class EOF(Abstract):
    pass

AbstractType.bind(EOF, 'GuEOF')


class In(Abstract):
    # BufferedIOBase methods
    def read(self, sz):
        arr = bytearray(sz)
        self.bytes(arr)
        return bytes(arr)

    def readinto(self, buf):
        slc = Slice.of_buffer(buf)
        self.bytes(slc)

    def read1(self, sz):
        raise UnsupportedOperation

    def write(b):
        raise UnsupportedOperation

    def detach(b):
        # Could be supported in principle.
        raise UnsupportedOperation
        
In.from_data = gu.data_in.static(In, dep(BytesCSlice), Pool.Out)
In.bytes = gu.in_bytes(None, In, Slice, Exn.Out)
In.new_buffered = gu.new_buffered_in.static(In, dep(In), c_size_t, Pool.Out)
BufferedIOBase.register(In)

@memo
def bridge(base):
    class Bridge(base):
        py = Field(py_object)
    Bridge.__name__ = base.__name__ + "Bridge"
    return Bridge

def wrap_method(base, attr):
    b = bridge(base)
    def wrapper(ref, *args):
        return getattr(pun(ref, b).py, attr)(*args)
    wrapper.__name__ = attr
    field = getattr(base, attr)
    return field.spec.c_type(wrapper)

class BridgeSpec(Spec):
    def to_c(self, x, ctx):
        b = bridge(self.c_type)
        br = b()
        for m in self.wrap_fields:
            meth = wrap_method(self.c_type, m)
            setattr(br, m, meth)
        br.py = self.wrapper(x)
        return br

    def to_py(c):
        return pun(c, bridge(self.c_type)).py


class InStream(CStructure, delay=True):
    pass

InStream._begin_buffer_dummy = Field(fn(None))
InStream._end_buffer_dummy = Field(fn(None))
InStream.input = Field(fn(c_size_t, ref(InStream), Slice, Exn.Out))
InStream.init()


class InStreamWrapper(Object):
    def input(self, slc):
        print("read %r" % len(slc))
        return self.stream.readinto(slc)

class InStreamBridge(instance(BridgeSpec)):
    c_type = InStream
    wrap_fields = ['input']
    def wrapper(i):
        return InStreamWrapper(stream=i)

In.new = gu.new_in.static(In, dep(ref(InStreamBridge)), Pool.Out)




class OutStream(CStructure, delay=True):
    pass

OutStream._begin_buffer_dummy = Field(fn(None))
OutStream._end_buffer_dummy = Field(fn(None))
OutStream.output = Field(fn(c_size_t, ref(OutStream), CSlice, Exn.Out))
OutStream.flush = Field(fn(None, ref(OutStream), Exn.Out))
OutStream.init()

c_output = wrap_method(OutStream, 'output')
c_flush = wrap_method(OutStream, 'flush')

class OutStreamWrapper(Object):
    def output(self, cslc):
        return self.stream.write(cslc)
    def flush(self):
        self.stream.flush()

class OutStreamBridge(instance(BridgeSpec)):
    c_type = OutStream
    wrap_fields = ['output', 'flush']
    def wrapper(o):
        return OutStreamWrapper(stream=o)

class Out(Abstract):
    def write(self, buf):
        self.bytes(buf)

Out.new = gu.new_out.static(Out, dep(ref(OutStreamBridge)), Pool.Out)
Out.bytes = gu.out_bytes(None, Out, CSlice, Exn.Out)
Out.flush = gu.out_flush(None, Out, Exn.Out)
    
BufferedIOBase.register(Out)



class WriterOutStreamWrapper(Object):
    def output(self, cslc):
        s,_ = utf_8_decode(cslc)
        return self.stream.write(s)
    def flush(self):
        self.stream.flush()

class WriterOutStreamBridge(instance(BridgeSpec)):
    c_type = OutStream
    wrap_fields = ['output', 'flush']
    def wrapper(o):
        return WriterOutStreamWrapper(stream=o)

class Writer(Abstract):
    def write(self, s):
        b, _ = utf_8_encode(s)
        utf8_write(b, self)

Writer.new = gu.new_writer.static(Writer, dep(ref(WriterOutStreamBridge)), Pool.Out)
Writer.flush = gu.writer_flush(None, Writer, Exn.Out)
utf8_write = gu.utf8_write.static(c_size_t, CSlice, Writer, Exn.Out)




#
#
#

class Enum(Abstract):
    def __iter__(self):
        return self
    def __next__(self):
        pool = Pool()
        ret = self._elem_spec.c_type()
        got = self.next(address(ret), pool)
        if not got:
            raise StopIteration
        # XXX: this isn't enough for structured data, e.g. pointers or
        # transparent structures.
        ret._add_dep(pool)
        return self._elem_spec.to_py(ret)

Enum.next = gu.enum_next(c_bool, Enum, c_void_p, Pool)

@memo
def enum(sot):
    class ElemEnum(Enum):
        _elem_spec = spec(sot)
    return ElemEnum

