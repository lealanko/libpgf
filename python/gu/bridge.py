from .core import *

@memo
def bridge(base):
    class Bridge(base):
        py = Field(py_object)
    Bridge.__name__ = base.__name__ + "Bridge"
    Bridge.__qualname__ = Bridge.__name__
    return Bridge

def wrap_method(base, funs, attr):
    #b = bridge(base)
    def wrapper(ref, *args):
        p = pun(ref, base)
        return getattr(p.py, attr)(*args)
    wrapper.__name__ = attr
    wrapper.__qualname__ = attr
    field = getattr(funs, attr)
    return field.spec.c_type(wrapper)

class BridgeSpec(Spec):
    @property
    @memo
    def bridge_table(self):
        """A table used to keep the bridge objects alive as long as the
        python-side target objects are alive."""
        return WeakDict()
    
    @property
    def c_type(self):
        return bridge(self.base_type)
    
    @memo
    def bridge_funs(self):
        funs = self.funs_type()
        for m in self.wrap_fields:
            meth = wrap_method(self.c_type, self.funs_type, m)
            setattr(funs, m, meth)
        return funs
    
    def to_c(self, x, ctx):
        try:
            return self.bridge_table[x]
        except KeyError:
            pass
        b = self.c_type
        br = b()
        br.funs = self.bridge_funs()
        br.py = self.wrapper(x)
        self.bridge_table[x] = br
        return br

    def to_py(c):
        return c.py

    def wrapper(self, x):
        return x

