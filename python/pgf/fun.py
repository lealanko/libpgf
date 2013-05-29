from pgf import *
import types

class _FunModule(types.ModuleType):
    __file__ = __file__
    def __init__(self, name, doc=None):
        import gu
        super().__init__(name, doc)
        self._pool = gu.Pool()

    def _close(self):
        self._pool = None
        
    def __getattr__(self, key):
        import gu, pgf
        with gu.Pool.default << self._pool:
            e = pgf.Expr.FUN(fun=key)
            self.__dict__[key] = e
            return e


sys.modules[__name__] = _FunModule(__name__)
