from .core import *
from .cstr import *


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
        try:
            name = self._name
        except AttributeError:
            pass
        else:
            self._c_type.__name__ = name
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

class _KindRefSpec(util.instance(ProxySpec)):
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

class _TypeSpec(util.instance(Spec)):
    c_type = POINTER(Type)

    def to_py(c):
        if not c:
            return None
        return get_ref(c[0].super.c_type, c)
    def to_c(x):
        return pointer(x)

class _TypeRefSpec(util.instance(Spec)):
    c_type = POINTER(Type)

    def to_py(c):
        if not c:
            return None
        return get_ref(c[0].super.c_type, c)
    def to_c(x):
        return pointer(x)

Type.Ref = _TypeRefSpec

Kind.bind(gu, 'type', Type)
Type.has_kind = gu.type_has_kind(c_bool, Type.Ref, Kind.Ref)


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
    def create_type(self):
        class SomeAbstract(Abstract):
            pass
        return SomeAbstract

Kind.bind(gu, 'abstract', AbstractType)

class OpaqueType(TypeRepr):
    pass

Kind.bind(gu, 'GuOpaque', OpaqueType)

