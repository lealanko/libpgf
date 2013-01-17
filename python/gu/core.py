from gupy import util
from gupy.ffi import *


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


class OpaqueMeta(CStructureMeta):
    def __call__(cls, *args):
        return cls.__new__(cls, *args)

class Opaque(CStructure, metaclass=OpaqueMeta):
    w_ = Field(c_uintptr)

