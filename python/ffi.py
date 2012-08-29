from _ctypes import dlsym, _Pointer as Pointer, \
    _SimpleCData as SimpleCData, CFuncPtr, FUNCFLAG_CDECL
from ctypes import *

from itertools import chain, repeat
from contextlib import contextmanager
from collections import OrderedDict

from util import *


for kind in [c_ushort, c_uint, c_ulong, c_ulonglong]:
    if sizeof(kind) == sizeof(c_void_p):
        c_uintptr = kind

c_uint8_p = POINTER(c_uint8)

_, CData, *_ = Pointer.__mro__

CPointerType = type(Pointer)
CFuncPtrType = type(CFuncPtr)
CStructType = type(Structure)
CDataType = type(CData)        



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

class Spec(Object):
    is_dep = False

    def from_addr(self, addr):
        return self.from_c(addr[c_type])

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

    def to_c(self, x):
        for a in self.spec.to_c(self.unwrap(x)):
            yield self.wrap_c(a)

    def from_c(self, c):
        return self.wrap(self.spec.from_c(self.unwrap_c(c)))

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
        self.dll = CDLL(soname)
        self.prefix = prefix

    def __getattr__(self, name):
        return Address(dlsym(self.dll._handle, self.prefix + name))

    def __getitem__(self, name):
        return Address(dlsym(self.dll._handle, name))

def address(cobj):
    return cast(byref(cobj), Address)

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

    def to_c(self, a):
        yield self.check(a)

    def from_c(self, a):
        return self.check(a)


def cspec(t):
    return CSpec(c_type = t)

def spec(sot):
    if isinstance(sot, Spec):
        return sot
    elif isinstance(sot, type) and issubclass(sot, CData):
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

    def from_c(self, c):
        if c:
            return self.spec.from_c(self.find_ref(c))
        else:
            return None

    def find_ref(self, c):
        return get_ref(self.spec.c_type, c)

    def to_c(self, r):
        if r is None:
            yield self.c_type()
        else:
            yield byref(r)

@memo        
def ref(sot):
    return RefSpec(sot=sot)


class CBase(CData):
    _deps = []
    _refspec = None

    def _add_dep(self, dep):
        if '_deps' not in self.__dict__:
            self._deps = []
        self._deps.append(dep)

class CFuncPtrBase(CBase):
    static = False
    def __get__(self, instance, owner):
        if instance and not self.static:
            return partial(self, instance)
        return self
    def __call__(self, *args):
        c_ret = None
        if len(args) > len(self.argspecs):
            raise TypeError('too many arguments')
        spec_it = zip(self.argspecs,
                      args + (None,) * (len(self.argspecs) - len(args)))
        c_args = []
        def wrap_args():
            nonlocal c_args, c_ret
            try:
                spec, arg = next(spec_it)
            except StopIteration:
                c_ret = CFuncPtr.__call__(self, *c_args)
                ret = self.resspec.from_c(c_ret)
                if isinstance(ret, CBase):
                    deps = []
                    for s, a in zip(self.argspecs, c_args):
                        if isinstance(s, Spec) and s.is_dep:
                            ret._add_dep(a)
                return ret
            with contextmanager(spec.to_c)(arg) as c:
                c_args.append(c)
                return wrap_args()
        return wrap_args()



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
    def __init__(self, t):
        self.type = t

class LazyField(Field):
    def __init__(self, f):
        self.f = f
    @property
    def type(self):
        return self.f()

class SpecField:
    def __init__(self, field, spec):
        self.field = field
        self.spec = spec
    def __get__(self, instance, owner):
        if not instance:
            return self
        c = self.field.__get__(instance, owner)
        return self.spec.from_c(c)
    def __set__(self, instance, value):
        c = self.field.__set__(instance, value)
        

class CStructureType(CStructType):
    def __prepare__(name, bases, **kwargs):
        return OrderedDict()

    def __init__(cls, name, parents, dict, delay=False):
        CStructType.__init__(cls, name, parents, dict)
    
    def __new__(cls, name, bases, odict, delay=False):
        fields = OrderedDict()
        d = dict()
        for k, v in odict.items():
            if isinstance(v, Field):
                fields[k] = spec(v.type)
            else:
                d[k] = v
        d['fields'] = fields
        ret = CStructType.__new__(cls, name, bases, d)
        if not delay:
            ret.init()
        return ret

    def __setattr__(self, name, value):
        if isinstance(value, Field):
            self.fields[name] = spec(value.type)
        else:
            CStructType.__setattr__(self, name, value)

    def init(self):
        self._fields_ = [(k, v.c_type) for k, v in self.fields.items()]
        for k, v in self.fields.items():
            f = getattr(self, k)
            setattr(self, k, SpecField(f, v))
        del self.fields

class CStructure(Structure, CBase, metaclass=CStructureType):
    pass

