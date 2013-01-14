from _ctypes import dlsym, _Pointer as Pointer, \
    _SimpleCData as SimpleCData, CFuncPtr, FUNCFLAG_CDECL
from ctypes import *

from itertools import chain, repeat
from contextlib import contextmanager
from collections import OrderedDict, Callable

from util import *


for kind in [c_ushort, c_uint, c_ulong, c_ulonglong]:
    if sizeof(kind) == sizeof(c_void_p):
        c_uintptr = kind

c_uint8_p = POINTER(c_uint8)

_ints = {}
_ints[sizeof(c_int)] = c_int
_ints[sizeof(c_long)] = c_long
_ints[sizeof(c_longlong)] = c_longlong


_uints = {}
_uints[sizeof(c_uint)] = c_uint
_uints[sizeof(c_ulong)] = c_ulong
_uints[sizeof(c_ulonglong)] = c_ulonglong

def sized_int(sz, signed=True):
    return _ints[sz] if signed else _uints[sz]

c_ssize_t = c_ptrdiff_t = sized_int(sizeof(c_size_t), signed=True)

_, CData, *_ = Pointer.__mro__

CPointerType = type(Pointer)
CFuncPtrType = type(CFuncPtr)
CStructType = type(Structure)
CDataType = type(CData)        
CArgObject = type(byref(c_int(0)))


class TypeRefTable(InternTable):
    def create(self, key):
        return self.ctype.from_address(key)

ref_table = WeakKeyDictionary()
def get_ref(ctype, addr):
    if not isinstance(addr, int):
        addr = cast(addr, c_void_p).value
    try:
        t = ref_table[ctype]
    except KeyError:
        ref_table[ctype] = t = TypeRefTable()
        t.ctype = ctype
    return t[addr]

dep_table = WeakDict()
def add_dep(key, value):
    dep_table.get(key, []).append(value)

def copy_deps(src, dst):
    try:
        deps = dep_table[src]
    except KeyError:
        pass
    else:
        dst_deps = dep_table.get(dst, [])
        dst_deps += deps

class Spec(Object):
    is_dep = False

    def from_addr(self, addr):
        return self.to_py(addr[c_type])

    def as_py(self, c, ctx):
        yield [self.to_py(c)]

    def as_c(self, r):
        yield self.to_c(r, None)

    def to_py(self, c):
        raise NotImplementedError

    def to_c(self, c, ctx):
        raise NotImplementedError

    def __invert__(cls):
        return ref(cls)


class ProxySpec(Spec):
    @property
    @memo
    def spec(self):
        return spec(self.sot)
    
    @property
    def is_dep(self):
        return self.spec.is_dep

    @property
    def c_type(self):
        return self.wrap_type(self.spec.c_type)

    def to_c(self, x, ctx):
        return self.wrap_c(self.spec.to_c(self.unwrap(x), ctx))

    def as_c(self, x):
        for a in self.spec.as_c(self.unwrap(x)):
            yield self.wrap_c(a)

    def to_py(self, c):
        return self.wrap(self.spec.to_py(self.unwrap_c(c)))

    def as_py(self, c, ctx):
        for l in self.spec.as_py(self.unwrap_c(c), ctx):
            yield [self.wrap(a) for a in l]

    def id_wrap(self, x):
        return x

    wrap_type = wrap = wrap_c = unwrap = unwrap_c = id_wrap

class DepSpec(ProxySpec):
    is_dep = True

@memo
def dep(sot):
    return DepSpec(sot=sot)

class DefaultSpec(ProxySpec):
    def unwrap(self, x):
        if x is None:
            return self.default()
        else:
            return x

def default(sot, dft):
    return DefaultSpec(sot=sot, default=dft)

class Address(c_void_p):
    def __getitem__(self, c_type):
        return get_ref(c_type, self)
    def __setitem__(self, c_type, value):
        cast(self, POINTER(c_type))[0] = value
    def __call__(self, res, *args):
        return cast(self, fn(res, *args))
    def static(self, res, *args):
        return cast(self, fn(res, *args, static=True))
    def __eq__(self, other):
        if not isinstance(other, Address):
            raise TypeError
        return self.value == other.value
    def __hash__(self):
        return hash(self.value)
    def __add__(self, offset):
        return Address(self.value + offset)



class Library:
    def __init__(self, soname, prefix):
        self.dll = CDLL(soname, mode=RTLD_GLOBAL)
        self.prefix = prefix

    def __getattr__(self, name):
        return Address(dlsym(self.dll._handle, self.prefix + name))

    def __getitem__(self, name):
        return Address(dlsym(self.dll._handle, name))

def address(cobj):
    return Address(addressof(cobj))

def pun(cobj, ctype):
    assert issubclass(ctype, type(cobj)) or isinstance(cobj, ctype)
    return ctype.from_address(addressof(cobj))

class CSpec(Spec):
    def check(self, a):
        if self.c_type is None:
            if a is not None:
                raise TypeError('value %r is not None', a)
        else:
            if not isinstance(a, self.c_type):
                try:
                    if issubclass(self.c_type, SimpleCData):
                        self.c_type(a)
                    else:
                        raise ValueError
                except ValueError:
                    raise TypeError('value %r is not an instance of %r', 
                                    a, self.c_type)
        return a

    def to_py(self, a):
        return self.check(a)

    def to_c(self, a, ctx):
        return self.check(a)

@memo
def cspec(t):
    return CSpec(c_type = t)

def spec(sot):
    if isinstance(sot, Spec):
        return sot
    elif isinstance(sot, type) and issubclass(sot, CData):
        try:
            return sot.default_spec
        except AttributeError:
            return cspec(sot)
    elif sot is None:
        return cspec(sot)
    else:
        raise TypeError('not a spec or type', sot)

class InternPointer(Pointer):
    def __getitem__(self, idx):
        if not isinstance(idx, int):
            return Pointer.__getitem__(self, idx)
        t = self._type_
        base = cast(self, c_void_p).value
        if base is None:
            return None
        a = base + idx * sizeof(t)
        return get_ref(t, a)

def IPOINTER(ctype):
    class IPointer(InternPointer):
        _type_ = ctype
    IPointer.__name__ = 'IP_' + ctype.__name__
    return IPointer


class RefSpec(ProxySpec):
    def wrap_type(self, c_type):
        return POINTER(c_type)

    def to_py(self, c):
        if c:
            return self.spec.to_py(self.find_ref(c))
        else:
            return None

    def as_py(self, c, ctx):
        if c:
            for a in self.spec.as_py(self.find_ref(c), ctx):
                yield a
        else:
            yield [None]

    def find_ref(self, c):
        return get_ref(self.spec.c_type, c)

    def to_c(self, r, ctx):
        if r is None:
            return self.c_type()
        else:
            return byref(r)

    def as_c(self, r):
        if r is None:
            yield self.c_type()
        else:
            for a in ProxySpec.as_c(self, r):
                yield byref(a)

#@memo        
def ref(sot):
    return RefSpec(sot=sot)

def cref(t):
    return ref(cspec(t))



    

class CBase(CData):
    _refspec = None
    def __hash__(self):
        return hash(addressof(self))

class Context:
    pass

class CFuncPtrBase(CBase):
    static = False

    def __new__(cls, *args):
        if len(args) == 1 and isinstance(args[0], Callable):
            return CFuncPtr.__new__(cls, partial(cls.callback, args[0]))
        else:
            return CFuncPtr.__new__(cls, *args)

    @classmethod
    def callback(cls, fn, *c_args):
        if len(c_args) != len(cls.argspecs):
            raise TypeError('illegal number of arguments')
        spec_it = zip(cls.argspecs, c_args)
        ctx = Context()
        def wrap_args(py_args):
            try:
                spec, arg = next(spec_it)
            except StopIteration:
                spec, arg = None, None
            if spec is None:
                py_ret = fn(*py_args)
                return cls.resspec.to_c(py_ret, ctx)
            else:
                with contextmanager(spec.as_py)(arg, ctx) as py:
                    return wrap_args(py_args + py)
        try:
            return wrap_args([])
        except Exception as e:
            return cls.resspec.c_type()
            
    def __get__(self, instance, owner):
        if instance and not self.static:
            return partial(self, instance)
        return self
    def __call__(self, *args):
        if len(args) > len(self.argspecs):
            raise TypeError('too many arguments')
        spec_it = zip(self.argspecs,
                      args + (None,) * (len(self.argspecs) - len(args)))
        def wrap_args(c_args):
            try:
                spec, arg = next(spec_it)
            except StopIteration:
                spec, arg = None, None
            if spec is None:
                c_ret = CFuncPtr.__call__(self, *c_args)
                ret = self.resspec.to_py(c_ret)
                for s, a in zip(self.argspecs, c_args):
                    if isinstance(s, Spec) and s.is_dep and ret is not None:
                        add_dep(ret, a)
                return ret
            else:
                with contextmanager(spec.as_c)(arg) as c:
                    return wrap_args(c_args + [c])
        return wrap_args([])



def fn(ressot, *argsots, static=False):
    s = static
    class CFunc(CFuncPtrBase, CFuncPtr):
        if s:
            static = True
        argspecs = [spec(s) for s in argsots]
        resspec = spec(ressot)
        _argtypes_ = [s.c_type for s in argspecs]
        _restype_ = resspec.c_type
        _flags_ = FUNCFLAG_CDECL
    return CFunc

class Field:
    def __init__(self, sot):
        self.sot = sot

class LazyField(Field):
    def __init__(self, f):
        self.f = f
    @property
    def type(self):
        return self.f()

class CField:
    def __init__(self, offset, sot=None, delay=None):
        self.offset = offset
        self.sot = sot
        self.delay = delay

    @property
    def c_type(self):
        if self.delay:
            sot = self.delay()
        else:
            sot = self.sot
        return spec(sot).c_type

    def __get__(self, instance, owner):
        if instance is None:
            return self
        return (address(instance) + self.offset)[self.c_type]

    def __set__(self, instance, value):
        (address(instance) + self.offset)[self.c_type] = value
        

class SpecField:
    def __init__(self, field, spec):
        self.field = field
        self.spec = spec
    def __get__(self, instance, owner):
        if instance is None:
            return self
        c = self.field.__get__(instance, owner)
        return self.spec.to_py(c)
    def __set__(self, instance, value):
        c = self.spec.to_c(value, None)
        if isinstance(c, CArgObject):
            c = pointer(c._obj)
        self.field.__set__(instance, c)
        

class CStructureMeta(CStructType):
    def __invert__(cls):
        return ref(cls)
    def __prepare__(name, bases, **kwargs):
        return OrderedDict()

    def __init__(cls, name, parents, dict, delay=False, **kwargs):
        CStructType.__init__(cls, name, parents, dict)
    
    def __new__(cls, name, bases, odict, delay=False, size=None, **kwargs):
        d = dict()
        if size is None:
            fields = OrderedDict()
            for k, v in odict.items():
                if isinstance(v, Field):
                    fields[k] = spec(v.sot)
                else:
                    d[k] = v
            d['_fields'] = fields
            ret = CStructType.__new__(cls, name, bases, d)
            if not delay:
                ret.init()
        else:
            if delay:
                raise ValueError
            d['_fields_'] = [('_data', c_byte * size)]
            d.update(odict)
            ret = CStructType.__new__(cls, name, bases, d)
        return ret

    def __setattr__(self, name, value):
        if isinstance(value, Field):
            self._fields[name] = spec(value.sot)
        else:
            CStructType.__setattr__(self, name, value)

    def init(self):
        self._fields_ = [(k, v.c_type) for k, v in self._fields.items()]
        for k, v in self._fields.items():
            f = getattr(self, k)
            setattr(self, k, SpecField(f, v))
        del self._fields

class CStructure(Structure, CBase, metaclass=CStructureMeta):
    pass



