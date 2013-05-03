from .core import *
from .bridge import *
from .pool import *
from .exn import *
from .out import *
from .slice import *

import codecs

class WriterOutStreamWrapper(Object):
    def output(self, cslc):
        s,_ = codecs.utf_8_decode(cslc)
        return self.stream.write(s)
    def flush(self):
        self.stream.flush()

class WriterOutStreamBridge(util.instance(BridgeSpec)):
    base_type = OutStream
    funs_type = OutStreamFuns
    wrap_fields = ['output', 'flush']
    def wrapper(o):
        return WriterOutStreamWrapper(stream=o)

class Writer(Abstract):
    def write(self, s):
        b, _ = codecs.utf_8_encode(s)
        utf8_write(b, self)

Writer.new = gu.new_writer.static(~Writer, dep(~WriterOutStreamBridge), Pool.Out)
Writer.flush = gu.writer_flush(None, ~Writer, Exn.Out)
utf8_write = gu.utf8_write.static(c_size_t, CSlice, ~Writer, Exn.Out)
