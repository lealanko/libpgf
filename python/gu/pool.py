from .core import *

class Pool(Abstract):
    alive = True
    own = False

    def __new__(cls):
        p = cls.new()
        p.own = True
        return p

    def _close(self):
        if self.alive:
            self._free()
            self.alive = False

    def __del__(self):
        if self.own:
            self._close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self._close()

Pool.new = gu.new_pool.static(~Pool)
Pool._free = gu.pool_free(None, ~Pool)

class _Pool_o(util.instance(CSpec)):
    c_type = IPOINTER(Pool)
    is_dep = True

    def as_c(p):
        if p is None:
            p = Pool()
        yield byref(p)

    def to_py(c):
        return c[0]

Pool.Out = dep(default(~Pool, lambda: Pool()))
    
