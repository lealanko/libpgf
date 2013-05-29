"""Simplified interface for parsing with PGF grammars."""

import gu
import pgf
import io
import inspect
from itertools import count
from functools import wraps, partial


def command(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        return Result(lambda: f(*args, **kwargs))
    wrapper.__signature__ = inspect.signature(f)
    return wrapper

def curry(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        if args:
            return f(*args, **kwargs)
        return partial(f, **kwargs)
    return wrapper
        

class Result:
    """An iterable that supports piping."""
    
    def __init__(self, thunk):
        self.thunk = thunk

    def __repr__(self):
        o = io.StringIO()
        print("<Result:", file=o)
        for i, r in enumerate(self):
            # Print components as str, not repr
            print(str(i) + ": " + str(r), file=o)
        print(">", end='', file=o)
        return o.getvalue()

    def __iter__(self):
        return self.thunk()

    @command
    def __or__(self, other):
        """Feed the values of this result into `other`, which must be a unary function. Returns a new
        :py:class:`Result` iterating over all the results from `other`."""
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
    """Load the current PGF grammar from `filename`. The currently
    active PGF grammar is implicitly used by the :py:func:`parse` and
    :py:func:`linearize` functions."""

    with open(filename, 'rb') as f:
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

@pgf.pooled
def _parse_tokens(parser, cat, tokens, pool):
    parse = parser.parse(cat, 0)
    for tok in tokens:
        parse = parse.token(tok)
    return parse.result(pool)

@curry
@command
@pgf.pooled
def parse(tokens, cat=None, lang=None, pool=None):
    """Parse `tokens`, which may be either a list or a string of space-separated tokens. Returns
    a :py:class:`Result` of :py:class:`pgf.Expr` syntax trees."""
    tokens = to_tokens(tokens)
    cat = to_cat(cat)
    if lang is None:
        concr = _current.parse_concr
    else:
        concr = to_concr(lang)
    parser = pgf.Parser.new(concr)
    for expr in _parse_tokens(parser, cat, tokens, pool=pool):
        yield expr


class _ListPresenter(list):
    def symbol_tokens(self, tokens):
        self += map(str, tokens)

@curry
@command
def linearize(expr, lang=None, flatten=True):
    """Linearize the expression `expr` into tokens. If `flatten` is
    true, the return result iterates over a string of space-separated
    tokens. Otherwise, the results are lists of tokens."""

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

