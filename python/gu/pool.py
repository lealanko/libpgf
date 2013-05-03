from .core import *

class Pool(Abstract):
    alive = True
    own = False

    def __new__(cls):
        p = cls._new()
        p.own = True
        return p

    def _close(self):
        if self.alive:
            self._free()
            self.alive = False

    def __del__(self):
        if self is not None and self.own:
            self._close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self._close()

    default = Parameter(None)

    @staticmethod
    def get():
        p = Pool.default()
        if p is None:
            p = Pool()
        return p

    @staticmethod
    def shift(pool=None):
        if pool is None:
            pool = Pool()
        return Pool.default << pool


def pooled(f):
    @wraps(f)
    def g(*args, pool=None, **kwargs):
        with Pool.shift() as p:
            if pool is None:
                pool = p
            return f(*args, pool=pool, **kwargs)
    return g
    

Pool._new = gu.new_pool.static(~Pool)
Pool._free = gu.pool_free(None, ~Pool)

class _Pool_o(util.instance(CSpec)):
    c_type = IPOINTER(Pool)
    is_dep = True

    def as_c(p):
        if p is None:
            p = Pool.get()
        yield byref(p)

    def to_py(c):
        return c[0]

Pool.Out = dep(default(~Pool, Pool.get))
