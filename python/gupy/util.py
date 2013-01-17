from weakref import *
from functools import wraps, partial
import collections

class classproperty(object):
  def __init__(self, f):
      self.f = f
  def __get__(self, instance, owner):
      return self.f(owner)

def memo(f):
    cache = WeakKeyDictionary()
    @wraps(f)
    def g(x):
        # This is a bit complicated to avoid calling f(x) in a handler.
        got = False
        try:
            val = cache[x]
            got = True
        except TypeError:
            weak = False
        except KeyError:
            weak = True
        if not got:
            val = f(x)
            if weak:
                cache[x] = val
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



class WeakKey(ref):
    def __init__(self, obj, remove):
        ref.__init__(self, obj, remove)
        self.id = id(obj)

    def __hash__(self):
        return self.id

    def __eq__(self, other):
        return self() is other() and self() is not None

class WeakDict(collections.MutableMapping):
    def __init__(self):
        self.dict = {}
        def remove(w):
            del self[w]
        self.remove = remove

    def wkey_(self, obj):
        return WeakKey(obj, self.remove)

    def __setitem__(self, key, value):
        wkey = self.wkey_(key)
        self.dict[wkey] = value

    def __getitem__(self, key):
        wkey = self.wkey_(key)
        return self.dict[wkey]

    def __delitem__(self, key):
        del self.dict[key]

    def get(self, key, value):
        try:
            return self[key]
        except KeyError:
            self[key] = value
            return value

    def __iter__(self):
        for w in iter(self.dict):
            v = w()
            if v is not None:
                yield v

    def __len__(self):
        return len(self.dict)
                
        
