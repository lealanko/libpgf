import gu
import pgf
import io
from itertools import count
from functools import wraps, partial


def command(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        return Result(lambda: f(*args, **kwargs))
    return wrapper

def curry(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        if args:
            return f(*args, **kwargs)
        return partial(f, **kwargs)
    return wrapper
        

class Result:
    def __init__(self, thunk):
        self.thunk = thunk

    def __repr__(self):
        o = io.StringIO()
        print("<Result:", file=o)
        for i, r in zip(count(0), self):
            # Print components as str, not repr
            print(str(i) + ": " + str(r), file=o)
        print(">", end='', file=o)
        return o.getvalue()

    def __iter__(self):
        return self.thunk()

    @command
    def __or__(self, other):
        for x in self:
            y = other(x)
            if isinstance(y, Result):
                for z in y:
                    yield z
            else:
                yield y


class Defaults:
    pgf = None
    cat = None

_current = Defaults()

# We need to clear _current (and run accompanying pool destructors)
# before modules are clared.

def _free_current():
    global _current
    _current = None

import atexit
atexit.register(_free_current)


def import_pgf(filename):
    f = open(filename, 'rb')
    i = gu.In.new(f)
    i = gu.In.new_buffered(i, 4096)
    _current.pgf = pgf.Pgf.read(i)
    _current.cat = _current.pgf.startcat()
    _current.concrs = list(_current.pgf.concrs())
    _current.parse_concr = _current.concrs[0]
    
def to_tokens(tokens):
    if isinstance(tokens, str):
        return tokens.split()
    return tokens

def to_cat(cat):
    if cat is None:
        return _current.cat
    if isinstance(cat, Cat):
        return cat
    if isinstance(cat, str):
        return _current.pgf.cat(cat)
    else:
        raise TypeError

def to_concr(l):
    if isinstance(l, pgf.Concr):
        return l
    elif isinstance(l, str):
        return _current.pgf.concr_by_lang(l)
    else:
        raise TypeError

def to_concrs(lang):
    if lang is None:
        return _current.concrs
    if isinstance(lang, str):
        lang = lang.split(',')
    return [to_concr(l) for l in lang]

def _parse_tokens(parser, cat, tokens, pool=None):
    if pool is None:
        pool = gu.Pool()
    parse = parser.parse(cat, 0, pool)
    for tok in tokens:
        parse = parse.token(tok, pool)
    return parse.result(pool)

@curry
@command
def parse(tokens, cat=None, lang=None):
    tokens = to_tokens(tokens)
    cat = to_cat(cat)
    if lang is None:
        concr = _current.parse_concr
    else:
        concr = to_concr(lang)
    parser = pgf.Parser.new(concr)
    for expr in _parse_tokens(parser, cat, tokens):
        yield expr

p = parse


class _ListPresenter(list):
    def symbol_tokens(self, tokens):
        self += tokens

@curry
@command
def linearize(expr, lang=None, flatten=True):
    if lang is None:
        concrs = _current.concrs
    else:
        concrs = to_concrs(lang)
    for concr in concrs:
        lzr = pgf.Lzr.new(concr)
        for ct in lzr.concretize(expr):
            pr = _ListPresenter()
            lzr.linearize(ct, 0, pr)
            if flatten:
                yield ' '.join(pr)
            else:
                yield list(pr)
        
l = linearize

