from .core import *
from .bridge import *
from .slice import *
from .pool import *
from .exn import *

import io

#
# In
#

class EOF(Abstract):
    pass

AbstractType.bind(gu, 'GuEOF', EOF)



class In(Abstract):
    # BufferedIOBase methods
    def read(self, sz):
        arr = bytearray(sz)
        self.bytes(arr)
        return bytes(arr)

    def readinto(self, buf):
        slc = Slice.of_buffer(buf)
        self.bytes(slc)

    def read1(self, sz):
        raise io.UnsupportedOperation

    def write(b):
        raise io.UnsupportedOperation

    def detach(b):
        # Could be supported in principle.
        raise io.UnsupportedOperation
        
In.from_data = gu.data_in.static(~In, dep(BytesCSlice), Pool.Out)
In.bytes = gu.in_bytes(None, ~In, Slice, Exn.Out)
In.new_buffered = gu.new_buffered_in.static(~In, dep(~In), c_size_t, Pool.Out)
io.BufferedIOBase.register(In)


class InStreamFuns(CStructure, delay=True):
    pass

class InStream(CStructure):
    funs = Field(ref(InStreamFuns))

InStreamFuns._begin_buffer_dummy = Field(fn(None))
InStreamFuns._end_buffer_dummy = Field(fn(None))
InStreamFuns.input = Field(fn(c_size_t, ~InStream, Slice, Exn.Out))
InStreamFuns.init()


class InStreamWrapper(Object):
    def input(self, slc):
        return self.stream.readinto(slc)

class InStreamBridge(util.instance(BridgeSpec)):
    c_type = InStream
    funs_type = InStreamFuns
    wrap_fields = ['input']
    def wrapper(i):
        return InStreamWrapper(stream=i)

In.new = gu.new_in.static(~In, dep(~InStreamBridge), Pool.Out)


