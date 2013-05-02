from .core import *
from .type import *
from .slist import *
from .pool import *
from io import StringIO

#
# Variants
#

class Constructor(CStructure):
    c_tag = Field(c_int)
    c_name = Field(CStr)
    type = Field(Type.Ref)


class Tag:
    def __init__(self, ctor, name, cls):
        self.cls = cls
        self.ctor = ctor
        self.name = name
        self.c_type = ctor.type.c_type
        self.spec = spec(self.c_type)

    def __str__(self):
        return self.name
        
    def __get__(self, instance, owner):
        if instance is None:
            return self
        if instance._tag() == self.ctor.c_tag:
            return [instance.data]
        else:
            return []

    def __call__(self, val=None, *args, pool=None, **kwargs):
        """Create a new variant instance with this tag and `val` as data."""
        if (args or kwargs) and val is None:
            c_val = self.c_type(*args, **kwargs)
        else:
            # XXX: make pool available in to_c
            c_val = self.spec.to_c(val, None)
        return self.cls.make(self.ctor.c_tag, c_val, pool)
        
        

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
            tag = Tag(ctor, sname, cls)
            cls._tags[ctor.c_tag] = tag
            cls._tags[sname] = tag
            setattr(cls, sname, tag)

            # Each variant class needs its own wrapper for
            # gu_make_variant since the return type is different and
            # memory management issues prevent switching to a new
            # wrapper afterwards.
            #
            # XXX: gu_make_variant copies the data into the variant, so
            # we are not really depending on the argument object itself.
            # However, we do depend on the things the argument object
            # depends on, so we have to add the dep for safety. TODO:
            # revise the variant system so that it can tag existing
            # objects (if their alignment is great enough) and doesn't
            # need to copy them.
            cls._make = gu.make_variant(
                cspec(cls),
                c_byte, c_size_t, c_size_t, dep(c_void_p), Pool.Out)
    
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
        if not hasattr(self, '_tags'):
            return '%s.%d(%x)' % (type(self).__name__,
                                  self._tag(),
                                  self._data())
        return '%s.%s(%s)' % (type(self).__name__,
                              self.tag.name,
                              repr(self.data))

    def __bool__(self):
        return self.tag is not None
    
    @property
    def tag(self):
        tagcode = self._tag()
        if tagcode < 0:
            return None
        return self._tags[tagcode]

    @property
    def data(self):
        tag = self.tag
        if not tag:
            return None
        p = cast(self._data(), POINTER(tag.c_type))
        return tag.spec.to_py(p[0])

    @classmethod
    def make(cls, tag, value, pool=None):
        #print(tag, alignment(value), sizeof(value), address(value), pool)
        v = cls._make(tag, alignment(value), sizeof(value),
                      address(value), pool)
        return v

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
        
