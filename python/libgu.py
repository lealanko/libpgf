from codecs import utf_8_encode, utf_8_decode, ascii_encode, ascii_decode
from io import RawIOBase, BufferedIOBase, TextIOBase, UnsupportedOperation, StringIO
from util import *
from ffi import *

gu = Library('libgu.so.0', "gu_")


# TODO: ABC
def dump(out, o, strict=False):
    if hasattr(o, 'dump'):
        o.dump(out, strict)
    else:
        out.write(repr(o))

def dump_kws(out, kws, strict=False):
    first = True
    for k, v in kws:
        if first:
            first = False
        else:
            out.write(', ')
        out.write(k + '=')
        dump(out, v, strict)

class AbstractMeta(CStructType):
    def __invert__(cls):
        return ref(cls)

class Abstract(Structure, CBase, metaclass=AbstractMeta):
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

Pool.new = gu.new_pool.static(~Pool)
Pool._free = gu.pool_free(None, ~Pool)


class CStr(instance(Spec)):
    c_type = c_char_p

    def to_py(c_val):
        s, _ = ascii_decode(c_val)
        return s
        

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

Pool.Out = dep(default(~Pool, lambda: Pool()))
    

class SListBase(CStructure):
    len = Field(c_int)

    def __getitem__(self, i):
        if i < 0 or i >= self.len:
            raise ValueError
        return self.elems[i]

    def __len__(self):
        return self.len

    def __iter__(self):
        for i in range(self.len):
            yield self[i]


@memo
def SList(elem_type):
    class SList(SListBase):
        elems = Field(POINTER(elem_type))
    return SList


##
## Types
##


class Kind(CStructure, delay=True):
    @classmethod
    def bind_(cls, lib, name, c_type=None):
        t = lib['gu_type__' + name][cls]
        t._name = name
        if c_type is not None:
            t._set_c_type(c_type)
        else:
            c_type = t.c_type
        try:
            old_gu_types = c_type.gu_types
        except AttributeError:
            old_gu_types = []
        c_type.gu_types = old_gu_types + [t]
        return t

    bind = bind_

    @classproperty
    @memo
    def spec(cls):
        return cref(cls)

    @property
    @memo
    def c_type(self):
        try:
            return self._c_type
        except AttributeError:
            pass
        self._c_type = self.create_type()
        return self._c_type
        
    def _set_c_type(self, c_type):
        try:
            prev_c_type = self._c_type
        except AttributeError:
            self._c_type = c_type
        else:
            if prev_c_type is not c_type:
                raise ValueError("Conflicting c_type %r for type object %r", c_type, self)

Kind.super = Field(~Kind)
Kind.init()

class _KindRefSpec(instance(ProxySpec)):
    sot = ~Kind
    def unwrap(py):
        return py.gu_types[0]
    def wrap(c):
        return c.c_type

Kind.Ref = _KindRefSpec    

class Type(Kind):
    @classmethod
    def bind(cls, lib, name, c_type=None):
        t = lib['gu_type__' + name][cls]
        return t.super.c_type.bind_(lib, name, c_type)

class _TypeRefSpec(instance(Spec)):
    c_type = POINTER(Type)

    def to_py(c):
        if not c:
            return None
        return get_ref(c[0].super.c_type, c)
    def to_c(x):
        return pointer(x)

Type.Ref = _TypeRefSpec

Kind.bind(gu, 'type', Type)
Type.has_kind = gu.type_has_kind(c_bool, Type.Ref, ~Kind)


class TypeRepr(Type):
    size = Field(c_uint16)
    align = Field(c_uint16)

Kind.bind(gu, 'repr', TypeRepr)

class PrimType(TypeRepr):
    name = Field(CStr)

Kind.bind(gu, 'primitive', PrimType)
# PrimType.bind(gu, 'void', None) # More trouble than it's worth.
Kind.bind(gu, 'integer', PrimType)
Kind.bind(gu, 'signed', PrimType)
PrimType.bind(gu, 'int', c_int)
Kind.bind(gu, 'GuFloating', PrimType)
PrimType.bind(gu, 'double', c_double)

class TypeAlias(Type):
    type = Field(Type.Ref)
    def create_type(self):
        return self.type.c_type

Kind.bind(gu, 'alias', TypeAlias)

class TypeDef(TypeAlias):
    name = Field(CStr)

Kind.bind(gu, 'typedef', TypeDef)    

class AbstractType(Type):
    pass

Kind.bind(gu, 'abstract', AbstractType)

class OpaqueType(TypeRepr):
    pass

Kind.bind(gu, 'GuOpaque', OpaqueType)

class Member(CStructure):
    offset = Field(c_ptrdiff_t)
    name = Field(CStr)
    type = Field(Type.Ref)
    is_flex = Field(c_bool)
    def as_field(self):
        return CField(self.offset, delay=lambda: self.type.c_type)

class StructBase(CStructure):

    def dump_kws(self, out, strict):
        kws = ((m.name, getattr(self, m.name)) for m in self._members)
        dump_kws(out, kws, strict)

    def dump(self, out, strict):
        out.write(type(self).__name__ + '(')
        self.dump_kws(out, strict)
        out.write(')')
            
    def __repr__(self):
        ms = ["%s=%s" % (m.name, repr(getattr(self, m.name)))
              for m in self._members]
        return "%s(%s)" % (type(self).__name__, ', '.join(ms))
        

class StructRepr(TypeRepr):
    name = Field(CStr)
    members = Field(SList(Member))

    def create_type(self):
        class cls(StructBase, size=self.size):
            _members = self.members
        cls.__name__ = self.name
        for m in self.members:
            setattr(cls, m.name, m.as_field())
        return cls
            


    

Kind.bind(gu, 'struct', StructRepr)




##
## Exceptions
##

class Exn(Abstract):
    def __new__(cls, exn=None, kind=None, pool=None):
        if kind is None:
            kind = Type
        return cls.new(exn, kind, pool)

Exn.new = gu.new_exn.static(~Exn, ~Exn, Kind.Ref, Pool.Out)
Exn.caught = gu.exn_caught(Type.Ref, ~Exn)
Exn.caught_data = gu.exn_caught_data(Address, ~Exn)

# Temporary wrapper until we integrate libgu exception types into the
# python exception hierarchy
class GuException(Exception):
    pass

class _CurrentExnSpec(instance(ProxySpec)):
    sot = ~Exn
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
                val = s.c_type.from_addr(addr)
                add_dep(val, e) # More precisely: the pool of e
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

class OpaqueMeta(CStructureMeta):
    def __call__(cls, *args):
        return cls.__new__(cls, *args)

class Opaque(CStructure, metaclass=OpaqueMeta):
    w_ = Field(c_uintptr)



#
# Variants
#

class Constructor(CStructure):
    c_tag = Field(c_int)
    c_name = Field(CStr)
    type = Field(Type.Ref)


class Tag:
    def __init__(self, ctor, name):
        self.ctor = ctor
        self.name = name
        self.c_type = ctor.type.c_type
        self.spec = spec(self.c_type)
        
    def __get__(self, instance, owner):
        if instance is None:
            return self
        if instance._tag() == self.ctor.c_tag:
            return [instance.data]
        else:
            return []

class VariantMeta(OpaqueMeta):
    def __init__(cls, name, parents, dict, ctors=None, prefix=None, **kwargs):
        OpaqueMeta.__init__(cls, name, parents, dict, **kwargs)
        if ctors is None:
            return
        cls._tags = {}
        for ctor in ctors:
            sname = ctor.c_name
            if prefix is not None and sname.startswith(prefix):
                sname = sname.replace(prefix, '', 1)
            tag = Tag(ctor, sname)
            cls._tags[ctor.c_tag] = tag
            cls._tags[sname] = tag
            setattr(cls, sname, tag)
    
class Variant(Opaque, metaclass=VariantMeta):
    def dump(self, out, strict):
        out.write('%s.%s(' % (type(self).__name__, self.tag))
        d = self.data
        if isinstance(d, StructBase) and not strict:
            d.dump_kws(out, strict)
        else:
            dump(out, d, strict)
        out.write(')')
    
    def __repr__(self):
        return '%s.%s(%s)' % (type(self).__name__,
                              self.tag,
                              repr(self.data))
    
    @property
    def tag(self):
        return self._tags[self._tag()].name

    @property
    def data(self):
        tag = self._tags[self._tag()]
        p = cast(self._data(), POINTER(tag.c_type))
        return tag.spec.to_py(p[0])

Variant._tag = gu.variant_tag(c_int, cspec(Variant))
Variant._data = gu.variant_data(c_void_p, cspec(Variant))


def pp_prefix(typename):
    buf = StringIO()
    init = True
    for c in typename:
        if c.isupper() and not init:
            buf.write('_')
        buf.write(c.upper())
        init = False
    buf.write('_')
    return buf.getvalue()

class VariantType(TypeRepr):
    ctors = Field(SList(Constructor))

    def create_type(self):
        try:
            name = self._name
        except AttributeError:
            name = None
            pfx = None
        else:
            pfx = pp_prefix(name)
        class SomeVariant(Variant, ctors=self.ctors, prefix=pfx):
            pass
        if name:
            SomeVariant.__name__ = self._name
        return SomeVariant

Kind.bind(gu, 'GuVariant', VariantType)    
        



#
# Enum  
# 

class EnumConstant(CStructure):
    name = Field(CStr)
    value = Field(c_int64)
    enum_value = Field(c_void_p) # pointer to some integer type

class EnumType(TypeRepr):
    constants = Field(SList(EnumConstant))

    def create_type(self):
        try:
            name = self._name
        except AttributeError:
            name = None
            pfx = None
        else:
            pfx = pp_prefix(name)
        have_neg = any(c.value < 0 for c in self.constants)
        repr_t = sized_int(self.size, signed=have_neg)
        class SomeEnum(repr_t):
            pass
        for c in self.constants:
            sname = c.name
            if pfx is not None and sname.startswith(pfx):
                sname = sname.replace(pfx, '', 1)
            setattr(SomeEnum, sname, c.value)
        if name:
            SomeEnum.__name__ = self._name
        return SomeEnum

Kind.bind(gu, 'enum', EnumType)    




#
# Seq
#

class Seq(Opaque):
    def _arr(self):
        arrtype = self.elem_spec.c_type * self.length()
        return cast(self._data(), POINTER(arrtype))[0]

    def _elemdata(self):
        return cast(self._data(), POINTER(self.elem_spec.c_type))

    def __getitem__(self, idx):
        return self.elem_spec.to_py(self._elemdata()[idx])

    def __setitem__(self, idx, val):
        self._elemdata()[idx] = self.elem_spec.to_c(val, None)

    def to_list(self):
        espec = self.elem_spec
        return [espec.to_py(x) for x in self._arr()]

    @classproperty
    @memo
    def default_spec(cls):
        return _SeqSpec(elem_spec=cls.elem_spec)

    @classmethod
    def from_list(cls, list, pool=None):
        espec = cls.elem_spec
        sz = sizeof(espec.c_type)
        n = len(list)
        seq = Seq._make(sz, n, pool)
        eseq = cls()
        eseq.w_ = seq.w_
        copy_deps(seq, eseq)
        data = eseq._elemdata()
        for i in range(n):
            data[i] = espec.to_c(list[i], None)
        return eseq
        
    @classproperty
    @memo
    def of(cls):
        @memo
        def of_(elem_sot):
            class ElemSeq(cls):
                elem_spec = spec(elem_sot)
            return ElemSeq
        return of_


Seq._data = gu.seq_data(c_void_p, cspec(Seq))
Seq.length = gu.seq_length(c_size_t, cspec(Seq))
Seq._make = gu.make_seq.static(cspec(Seq), c_size_t, c_size_t, Pool.Out)

class Bytes(Seq.of(cspec(c_byte))):
    def bytes(self):
        return bytes(self._arr())

class _SeqSpec(Spec):
    @property
    def c_type(self):
        return Seq.of(self.elem_spec)
    def to_py(self, c):
        return c.to_list()
    def to_c(self, x, ctx):
        return self.c_type.from_list(x)
    


#
# SeqType
#

class SeqType(OpaqueType):
    elem_type = Field(Type.Ref)
    def create_type(self):
        return Seq.of(self.elem_type)


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

    def dump(self, out, strict=False):
        if strict:
            out.write('String(')
        out.write(repr(str(self)))
        if strict:
            out.write(')')

    def __repr__(self):
        return "String(%r)" % str(self)

    def __eq__(self, other):
        return self.eq(other)

String.from_utf8 = gu.utf8_string.static(cspec(String), BytesCSlice, Pool.Out)
String.utf8 = gu.string_utf8(Slice, cspec(String), Pool.Out)

OpaqueType.bind(gu, 'GuString', String)


class _StringSpec(instance(ProxySpec)):
    sot = cspec(String)
    def unwrap(s):
        return String(s) if isinstance(s, str) else s
    def wrap(s):
        return str(s)

String.default_spec = _StringSpec
String.eq = gu.string_eq(c_bool, String, String)

Strings = Seq.of(String)


#
# In
#

class EOF(Abstract):
    pass

AbstractType.bind(gu, 'GuEOF', EOF)





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
        
In.from_data = gu.data_in.static(~In, dep(BytesCSlice), Pool.Out)
In.bytes = gu.in_bytes(None, ~In, Slice, Exn.Out)
In.new_buffered = gu.new_buffered_in.static(~In, dep(~In), c_size_t, Pool.Out)
BufferedIOBase.register(In)

@memo
def bridge(base):
    class Bridge(base):
        py = Field(py_object)
    Bridge.__name__ = base.__name__ + "Bridge"
    return Bridge

def wrap_method(base, funs, attr):
    b = bridge(base)
    def wrapper(ref, *args):
        return getattr(pun(ref, b).py, attr)(*args)
    wrapper.__name__ = attr
    field = getattr(funs, attr)
    return field.spec.c_type(wrapper)

class BridgeSpec(Spec):
    @memo
    def bridge_funs(self):
        funs = self.funs_type()
        for m in self.wrap_fields:
            meth = wrap_method(self.c_type, self.funs_type, m)
            setattr(funs, m, meth)
        return funs
    
    def to_c(self, x, ctx):
        b = bridge(self.c_type)
        br = b()
        br.funs = self.bridge_funs()
        br.py = self.wrapper(x)
        return br

    def to_py(c):
        return pun(c, bridge(self.c_type)).py

    def wrapper(self, x):
        return x


class InStreamFuns(CStructure, delay=True):
    pass

class InStream(CStructure):
    funs = Field(ref(InStreamFuns))

InStreamFuns._begin_buffer_dummy = Field(fn(None))
InStreamFuns._end_buffer_dummy = Field(fn(None))
InStreamFuns.input = Field(fn(c_size_t, ~InStream, Slice, Exn.Out))
InStreamFuns.init()


class InStreamWrapper(Object):
    def input(self, slc):
        return self.stream.readinto(slc)

class InStreamBridge(instance(BridgeSpec)):
    c_type = InStream
    funs_type = InStreamFuns
    wrap_fields = ['input']
    def wrapper(i):
        return InStreamWrapper(stream=i)

In.new = gu.new_in.static(~In, dep(~InStreamBridge), Pool.Out)




class OutStreamFuns(CStructure, delay=True):
    pass

class OutStream(CStructure):
    funs = Field(~OutStreamFuns)

OutStreamFuns._begin_buffer_dummy = Field(fn(None))
OutStreamFuns._end_buffer_dummy = Field(fn(None))
OutStreamFuns.output = Field(fn(c_size_t, ~OutStream, CSlice, Exn.Out))
OutStreamFuns.flush = Field(fn(None, ~OutStream, Exn.Out))
OutStreamFuns.init()

class OutStreamWrapper(Object):
    def output(self, cslc):
        return self.stream.write(cslc)
    def flush(self):
        self.stream.flush()

class OutStreamBridge(instance(BridgeSpec)):
    c_type = OutStream
    funs_type = OutStreamFuns
    wrap_fields = ['output', 'flush']
    def wrapper(o):
        return OutStreamWrapper(stream=o)

class Out(Abstract):
    def write(self, buf):
        self.bytes(buf)

Out.new = gu.new_out.static(Out, dep(~OutStreamBridge), Pool.Out)
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
    funs_type = OutStreamFuns
    wrap_fields = ['output', 'flush']
    def wrapper(o):
        return WriterOutStreamWrapper(stream=o)

class Writer(Abstract):
    def write(self, s):
        b, _ = utf_8_encode(s)
        utf8_write(b, self)

Writer.new = gu.new_writer.static(~Writer, dep(~WriterOutStreamBridge), Pool.Out)
Writer.flush = gu.writer_flush(None, ~Writer, Exn.Out)
utf8_write = gu.utf8_write.static(c_size_t, CSlice, ~Writer, Exn.Out)




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
        add_dep(ret, pool)
        return self._elem_spec.to_py(ret)

Enum.next = gu.enum_next(c_bool, ~Enum, c_void_p, ~Pool)

@memo
def enum(sot):
    class ElemEnum(Enum):
        _elem_spec = spec(sot)
    return ElemEnum

