from .core import *
from .type import *
from .slist import *
from io import StringIO

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
        return SomeVariant

Kind.bind(gu, 'GuVariant', VariantType)    
        
