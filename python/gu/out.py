from .core import *
from .bridge import *
from .slice import *
from .pool import *
from .exn import *

import io

class OutStreamFuns(CStructure, delay=True):
    pass

class OutStream(CStructure):
    funs = Field(~OutStreamFuns)

OutStreamFuns._begin_buffer_dummy = Field(fn(None))
OutStreamFuns._end_buffer_dummy = Field(fn(None))
OutStreamFuns.output = Field(fn(c_size_t, ~OutStream, CSlice, Exn.Out))
OutStreamFuns.flush = Field(fn(None, ~OutStream, Exn.Out))
OutStreamFuns.init()

class OutStreamWrapper(Object):
    def output(self, cslc):
        return self.stream.write(cslc)
    def flush(self):
        self.stream.flush()

class OutStreamBridge(util.instance(BridgeSpec)):
    c_type = OutStream
    funs_type = OutStreamFuns
    wrap_fields = ['output', 'flush']
    def wrapper(o):
        return OutStreamWrapper(stream=o)

class Out(Abstract):
    def write(self, buf):
        self.bytes(buf)

Out.new = gu.new_out.static(Out, dep(~OutStreamBridge), Pool.Out)
Out.bytes = gu.out_bytes(None, Out, CSlice, Exn.Out)
Out.flush = gu.out_flush(None, Out, Exn.Out)
    
io.BufferedIOBase.register(Out)

