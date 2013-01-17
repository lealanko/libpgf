from .type import *
from .slist import *

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


