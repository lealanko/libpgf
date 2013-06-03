"""Syntactic sugar for identifiers in GF syntax trees.

This pseudo-module provides a simple way to create GF identifiers. Simply use::

   from pgf.fun import Foo, Bar, Baz

This is nearly equivalent to::

   Foo = pgf.Expr.FUN(fun='Foo')
   Bar = pgf.Expr.FUN(fun='Bar')
   Baz = pgf.Expr.FUN(fun='Baz')

The difference is that the objects in ``pgf.fun`` are allocated from a
dedicated memory pool that lives until the end of the program.

It is naturally also possible to access the members of module as
``pgf.fun.Foo`` etc.

"""

import sys
import types
__all__ = []

# This is all rather hacky. It would be better to have a custom loader.
class _FunModule(types.ModuleType):
    _orig_module = sys.modules[__name__]
    def __init__(self, name, doc):
        import gu
        import atexit
        super().__init__(name, doc)
        self._pool = gu.Pool()
        # This is needed to ensure that the pool is freed before core
        # modules get torn down.
        atexit.register(gu.Pool._close, self._pool)

    def __getattr__(self, key):
        if key.startswith('__'):
            return getattr(self._orig_module, key)
        elif key.startswith('_'):
            raise AttributeError
        import gu, pgf
        with gu.Pool.default << self._pool:
            e = pgf.Expr.FUN(fun=key)
            self.__dict__[key] = e
            return e

for k, v in list(globals().items()):
    if k.startswith('__'):
        setattr(_FunModule, k, v)

sys.modules[__name__] = _FunModule(__name__, __doc__)
