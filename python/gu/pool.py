"""Memory pools.

The ``libgu`` library uses pool-based memory management: a number of
objects are allocated from a single memory pool, until the entire pool
is freed along with all its objects.

The Python bindings try to provide a safe and convenient interface to
hide the complexity of the pools, but for performance reasons the pools
may still require manual management. The conventions for memory pools
are as follows:

All functions that allocate ``libgu`` objects have an optional ``pool``
keyword parameter. If a `Pool` object is provided as an
argument, the objects are allocated from that pool.

If the ``pool`` argument is not provided (or ``None``), then the
*default* pool is used. The default pool is accessed through
`Pool.default`, which is a thread-local
`gupy.util.Parameter` object. Some common operations on the
default pool are encapsulated in the `Pool.shift` method and the
`pooled` and `ensure_pool` wrappers.

If the default pool, too, is ``None`` (as it is initially), then a new
pool is created when ``libgu`` needs to allocate something. This is
convenient, but results in a proliferation of small pools, which is
inefficient.

All Python-side `libgu` objects retain a reference to the pool from
which they were allocated (as well as other objects they depend on), so
the user of the library is not forced to worry about object lifetimes.
However, memory efficiency suffers if large pools are kept alive due to
lingering references to few small objects in them.

"""

from .core import *

class Pool(Abstract):
    """A memory pool."""

    alive = True
    own = False

    def __new__(cls):
        p = cls._new()
        p.own = True
        return p

    def _close(self):
        if self.alive:
            # bypass normal machinery to cope with program shutdown
            from _ctypes import CFuncPtr, byref
            CFuncPtr.__call__(type(self)._free, byref(self))
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
        """A context with a new pool.

        Returns a context manager that sets the default pool to ``pool``
        or, if ``None``, a newly allocated pool. The context manager
        yields the previous default pool. When the context exits, the
        previous pool is restored as default.
        """

        if pool is None:
            pool = Pool()
        return Pool.default << pool


def pooled(f):
    """Execute the function with a newly allocated default pool."""
    @wraps(f)
    def g(*args, pool=None, **kwargs):
        with Pool.shift() as p:
            if pool is None:
                pool = p
            return f(*args, pool=pool, **kwargs)
    return g


def ensure_pool(f):
    """Ensure that there is a default pool while executing the function."""
    @wraps(f)
    def g(*args, **kwargs):
        with Pool.shift(Pool.default()):
            return f(*args, **kwargs)
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
