from .core import *
from .type import *
from .pool import *

#
# Seq
#

class Seq(Opaque):
    def _arr(self):
        arrtype = self.elem_spec.c_type * self.length()
        return cast(self._data(), POINTER(arrtype))[0]

    def _elemdata(self):
        return cast(self._data(), POINTER(self.elem_spec.c_type))

    def __getitem__(self, idx):
        return self.elem_spec.to_py(self._elemdata()[idx])

    def __setitem__(self, idx, val):
        self._elemdata()[idx] = self.elem_spec.to_c(val, None)

    def to_list(self):
        espec = self.elem_spec
        return [espec.to_py(x) for x in self._arr()]

    @classproperty
    @memo
    def default_spec(cls):
        return _SeqSpec(elem_spec=cls.elem_spec)

    @classmethod
    def from_list(cls, list, pool=None):
        espec = cls.elem_spec
        sz = sizeof(espec.c_type)
        n = len(list)
        seq = Seq._make(sz, n, pool)
        eseq = cls()
        eseq.w_ = seq.w_
        copy_deps(seq, eseq)
        data = eseq._elemdata()
        for i in range(n):
            data[i] = espec.to_c(list[i], None)
        return eseq
        
    @classproperty
    @memo
    def of(cls):
        @memo
        def of_(elem_sot):
            class ElemSeq(cls):
                elem_spec = spec(elem_sot)
            return ElemSeq
        return of_


Seq._data = gu.seq_data(c_void_p, cspec(Seq))
Seq.length = gu.seq_length(c_size_t, cspec(Seq))
Seq._make = gu.make_seq.static(cspec(Seq), c_size_t, c_size_t, Pool.Out)

class Bytes(Seq.of(cspec(c_byte))):
    def bytes(self):
        return bytes(self._arr())

class _SeqSpec(Spec):
    @property
    def c_type(self):
        return Seq.of(self.elem_spec)
    #def to_py(self, c):
    #    return c.to_list()
    def to_c(self, x, ctx):
        if isinstance(x, self.c_type):
            return x
        return self.c_type.from_list(x)
        
    


#
# SeqType
#

class SeqType(OpaqueType):
    elem_type = Field(Type.Ref)
    def create_type(self):
        return Seq.of(self.elem_type.c_type)

Kind.bind(gu, 'GuSeq', SeqType)

