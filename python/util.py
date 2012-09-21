from weakref import *
from functools import wraps, partial

class classproperty(object):
  def __init__(self, f):
      self.f = f
  def __get__(self, instance, owner):
      return self.f(owner)

def memo(f):
    cache = WeakKeyDictionary()
    @wraps(f)
    def g(x):
        try:
            val = cache[x]
        except TypeError:
            val = f(x)
        except KeyError:
            val = cache[x] = f(x)
        return val
    return g

class Instance(type):
    def __new__(cls, name, parents, d, initial=False):
        if initial:
            return type.__new__(cls, name, parents, d)
        elif not parents:
            c = Object
        else:
            c = parents[0]
        if hasattr(c, 'instance_class'):
            c = c.instance_class
        return c(**d)
    def __init__(self, name, parents, d, initial=False):
        type.__init__(self, name, parents, d)

@memo
def instance(cls):
    class C(cls, metaclass=Instance, initial=True):
        instance_class = cls
    return C

class Object:
    def __init__(self, **kwargs):
        self.__dict__ = kwargs


class InternTable(WeakValueDictionary):
    def __getitem__(self, key):
        try:
            return WeakValueDictionary.__getitem__(self, key)
        except KeyError:
            value = self.create(key)
            self[key] = value
            return value

