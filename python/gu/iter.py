from .core import *
from .pool import *

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

