from .core import *

class SListBase(CStructure):
    len = Field(c_int)

    def __getitem__(self, i):
        if i < 0 or i >= self.len:
            raise ValueError
        return self.elems[i]

    def __len__(self):
        return self.len

    def __iter__(self):
        for i in range(self.len):
            yield self[i]


@memo
def SList(elem_type):
    class SList(SListBase):
        elems = Field(POINTER(elem_type))
    return SList
