from .core import *

@memo
def bridge(base):
    class Bridge(base):
        py = Field(py_object)
    Bridge.__name__ = base.__name__ + "Bridge"
    return Bridge

def wrap_method(base, funs, attr):
    b = bridge(base)
    def wrapper(ref, *args):
        return getattr(pun(ref, b).py, attr)(*args)
    wrapper.__name__ = attr
    field = getattr(funs, attr)
    return field.spec.c_type(wrapper)

class BridgeSpec(Spec):
    @memo
    def bridge_funs(self):
        funs = self.funs_type()
        for m in self.wrap_fields:
            meth = wrap_method(self.c_type, self.funs_type, m)
            setattr(funs, m, meth)
        return funs
    
    def to_c(self, x, ctx):
        b = bridge(self.c_type)
        br = b()
        br.funs = self.bridge_funs()
        br.py = self.wrapper(x)
        return br

    def to_py(c):
        return pun(c, bridge(self.c_type)).py

    def wrapper(self, x):
        return x

