from .core import *
from .pool import *
from .type import *

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

class _CurrentExnSpec(util.instance(ProxySpec)):
    sot = ~Exn
    optional = True
    def as_c(e):
        if e is None:
            e = Exn()
        for c in Exn.Out.spec.as_c(e):
            yield c
        t = e.caught()
        if t:
            addr = e.caught_data()
            s = spec(t.c_type)
            val = None
            if addr:
                val = s.to_py(addr[t.c_type])
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

    

