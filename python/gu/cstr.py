from codecs import ascii_decode
from .core import *

class CStr(util.instance(Spec)):
    c_type = c_char_p

    def to_py(c_val):
        s, _ = ascii_decode(c_val)
        return s
