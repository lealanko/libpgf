from .core import *
from .type import *
from .seq import *
from .slice import *

import codecs

#
# String
#

class String(Opaque):
    def __new__(cls, s, pool = None):
        b, _ = codecs.utf_8_encode(s)
        return cls.from_utf8(b, pool)

    def __str__(self):
        slc = self.utf8()
        s, _ = codecs.utf_8_decode(slc)
        return s

    def dump(self, out, strict=False):
        if strict:
            out.write('String(')
        out.write(repr(str(self)))
        if strict:
            out.write(')')

    def __repr__(self):
        return "String(%r)" % str(self)

    def __eq__(self, other):
        return self.eq(other)

String.from_utf8 = gu.utf8_string.static(cspec(String), BytesCSlice, Pool.Out)
String.utf8 = gu.string_utf8(Slice, cspec(String), Pool.Out)
String.is_null = gu.string_is_null(c_bool, cspec(String))

String.null = gu.null_string[String]

OpaqueType.bind(gu, 'GuString', String)


class _StringSpec(util.instance(ProxySpec)):
    sot = cspec(String)
    def unwrap(s):
        if s is None:
            return String.null
        elif isinstance(s, str):
            return String(s)
        return s
    def wrap(s):
        if s.is_null():
            return None
        return str(s)

String.default_spec = _StringSpec
String.eq = gu.string_eq(c_bool, String, String)

Strings = Seq.of(String)

