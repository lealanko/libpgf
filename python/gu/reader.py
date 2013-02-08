from .core import *
from .bridge import *
from .slice import *
from .pool import *
from .exn import *
from .ucs import *
from .in_ import *

from codecs import utf_8_encode
import io

class Reader(Abstract):
    encoding = None
    newlines = None
    buffer = None

    def detach(self):
        raise UnsupportedOperation

    def _read(self, limit=None, delim=[]):
        if limit is not None and limit < 0:
            limit = None
        buf = io.StringIO()
        n = 0
        try:
            while limit is None or n < limit:
                c = self.read_ucs()
                buf.write(c)
                if c in delim:
                    break
                n += 1
        except GuException as e:
            # TODO: integrate libgu exception types to standard python
            # exception classes
            if e.args[0].has_kind(GuEOF):
                pass
            else:
                raise e
        return buf.getvalue()

    def read(self, n=None):
        return self._read(n)
    
    def readline(self, limit=-1):
        return self._read(limit, '\n')

    def seek(self, offset, whence):
        raise UnsupportedOperation

    def tell(self):
        raise UnsupportedOperation

    def write(self, s):
        raise UnsupportedOperation

    @staticmethod
    def new_textio(textin, pool=None):
        bin_in = Utf8InIO(textin)
        instream = In.new(bin_in, pool)
        return Reader.new_utf8(instream, pool)

    @staticmethod
    def new_string(str, pool=None):
        sio = io.StringIO(str)
        return Reader.new_textio(sio, pool)
        



Reader.new_utf8 = gu.new_utf8_reader.static(~Reader, dep(~In), Pool.Out)
Reader.read_ucs = gu.read_ucs(UCS, ~Reader, Exn.Out)
io.TextIOBase.register(Reader)


# XXX: redo the reader mess, this is just silly
class Utf8InIO:
 at = 0
 buf = b''

 def readinto(self, slc):
     rem = len(self.buf) - self.at
     if not rem:
         s = self.textin.read(1)
         self.buf, _ = utf_8_encode(s)
         self.at = 0
         rem = len(self.buf)
     l = min(rem, len(slc))
     slc[:l] = self.buf[self.at:self.at+l]
     self.at += l
     return l

 def __init__(self, textin):
     self.textin = textin

