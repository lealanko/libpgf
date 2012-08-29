from codecs import utf_8_encode, utf_8_decode

from util import *
from ffi import *

gu = Library('libgu.so.0', "gu_")

class Abstract(Structure, CBase):
    pass

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

Pool.new = gu.new_pool.static(ref(Pool))
Pool._free = gu.pool_free(None, ref(Pool))


class CSlice(CStructure):
    p = Field(c_uint8_p)
    sz = Field(c_size_t)
    def bytes(self):
        return string_at(self.p, self.sz)
    def __getitem__(self, idx):
        if idx < 0 or idx >= self.sz:
            raise ValueError
        return self.p[idx]

class Slice(CSlice):
    def __setitem__(self, idx, value):
        if idx < 0 or ifx >= self.sz:
            raise ValueError
        self.p[idx] = value
    def array(self):
        return cast(self.p, Address)[c_byte * self.sz]

class BytesCSlice(instance(Spec)):
    c_type = CSlice

    def from_c(c_val):
        return c_val.bytes()
    
    def to_c(b):
        yield CSlice(p=cast(b, c_uint8_p), sz=len(b))

class ArraySlice(instance(Spec)):
    c_type = Slice

    def from_c(c_val):
        return c_val.array()

    def to_c(a):
        yield Slice(p=cast(a, c_uint8_p), sz=len(a))


class _Pool_o(instance(CSpec)):
    c_type = IPOINTER(Pool)
    is_dep = True

    def to_c(p):
        if p is None:
            p = Pool()
        yield byref(p)

    def from_c(c):
        return c[0]

Pool.out = dep(default(ref(Pool), lambda: Pool()))
    



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
        return type_spec(ref(cls))

    # TODO: autocreate c_type for GuTypes that haven't been explicitly
    # bound
        

Kind.super = Field(ref(Kind))
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

Exn.new = gu.new_exn.static(ref(Exn), ref(Exn), Kind.spec, Pool.out)
Exn.caught = gu.exn_caught(Type.spec, ref(Exn))
Exn.caught_data = gu.exn_caught_data(Address, ref(Exn))

# Temporary wrapper until we integrate libgu exception types into the
# python exception hierarchy
class GuException(Exception):
    pass

class ExnSpec(instance(ProxySpec)):
    sot = ref(Exn)
    def to_c(e):
        if e is None:
            e = Exn()
        for c in ExnSpec.spec.to_c(e):
            yield c
        t = e.caught()
        if t:
            addr = e.caught_data()
            s = spec(t)
            if addr:
                val = s.from_addr(addr)
                val.add_dep(e) # More precisely: the pool of e
            else:
                val = s.c_type
            raise GuException(val)

class ExnData(CStructure):
    pool = Field(ref(Pool))
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
        return self.elemspec.from_c(self._elemdata()[idx])

    def __setitem__(self, idx, val):
        self._elemdata()[idx] = self.elemspec.to_c(val)

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
        b = self.utf8()
        s, _ = utf_8_decode(b._arr())
        return s

    def __eq__(self, other):
        return self.eq(other)

String.from_utf8 = gu.utf8_string.static(String, BytesCSlice, Pool.out)
String.utf8 = gu.string_utf8(Bytes, String, Pool.out)
String.eq = gu.string_eq(c_bool, String, String)



#
# In
#

class EOF(Abstract):
    pass

AbstractType.bind(EOF, 'GuEOF')


class In(Opaque):
    def bytes(self, sz):
        arr = create_string_buffer(sz)
        self._bytes(arr)
        return bytes(arr)

In.from_data = gu.data_in.static(ref(In), dep(BytesCSlice), Pool.out)
In._bytes = gu.in_bytes(None, ref(In), ArraySlice, ExnSpec)


