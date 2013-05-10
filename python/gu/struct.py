from .core import *
from .type import *
from .slist import *

class Member(CStructure):
    offset = Field(c_ptrdiff_t)
    name = Field(CStr)
    type = Field(Type.Ref)
    is_flex = Field(c_bool)
    def as_field(self):
        f = CField(self.offset, delay=lambda: self.type.c_type)
        return SpecField(f, thunk=lambda: spec(self.type.c_type))

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
        class cls(StructBase, size=self.size, align=self.align):
            _members = self.members
        # XXX: this name is just informative, it cannot be used as an identifier.
        # TODO: maybe create a dynamic module where these can be imported?
        cls.__name__ = self.name
        cls.__qualname__ = cls.__name__
        for m in self.members:
            setattr(cls, m.name, m.as_field())
        return cls
            
Kind.bind(gu, 'struct', StructRepr)
