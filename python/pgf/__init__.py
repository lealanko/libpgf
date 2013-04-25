"""Python interface to the `libpgf` library."""

from gu import *

pgf = Library('libpgf.so.0', "pgf_")

CId = String
Token = String
CtntId = c_int

class Cat(Abstract):
    """A syntactic category in an abstract grammar."""

class Concr(Abstract, metaclass=delay_init):
    """A concrete grammar."""
    
    @cfunc(pgf.concr_id)
    def id(self: ~Concr) -> String:
        """Return the internal name of this concrete grammar."""

    @cfunc(pgf.concr_lang)
    def lang(self: ~Concr) -> String:
        """Return the language code for this concrete grammar."""

ConcrEnum = enum(~Concr)

class Pgf(Abstract, metaclass=delay_init):
    """A PGF grammar."""
    
    @cfunc(pgf.read_pgf, static=True)
    def read(input: ~In, pool: Pool.Out, exn: Exn.Out) -> ~Pgf:
        """Read the PGF file `input` and return a new PGF grammar object."""

    @cfunc(pgf.pgf_startcat)
    def startcat(self: dep(~Pgf)) -> ~Cat:
        """Return the default starting category."""

    @cfunc(pgf.pgf_cat)
    def cat(self: dep(~Pgf), id: CId) -> ~Cat:
        """Return the category with the name `id`."""

    @cfunc(pgf.pgf_concr)
    def concr(self: dep(~Pgf), id: CId, pool: Pool.Out) -> ~Concr:
        """Return the concrete grammar with the name `id`."""

    @cfunc(pgf.pgf_concr_by_lang)
    def concr_by_lang(self: dep(~Pgf), lang: String, pool: Pool.Out
                      ) -> ~Concr:
        """Return the concrete grammar for the language `lang`."""

    @cfunc(pgf.pgf_concrs)
    def concrs(self: dep(~Pgf), pool: Pool.Out) -> ~ConcrEnum:
        """Return an iterator over all the concrete grammars."""
    
    



class ReadExn(Abstract):
    pass

AbstractType.bind(pgf, 'PgfReadExn', ReadExn)

ExprType = Type.bind(pgf, 'PgfExpr')

Expr = ExprType.c_type
#Expr.read = pgf.read_expr.static(Expr, ~Reader, Pool.Out, Exn.Out)
#Expr.print = pgf.expr_print(None, Expr, ~Writer, Exn.Out)


class Expr(ExprType.c_type, metaclass=initialize):
    """A syntax tree of an abstract grammar."""

    @cfunc(pgf.read_expr, static=True)
    def read(reader: ~Reader, pool: Pool.Out, exn: Exn.Out) -> Expr:
        """Read an expression from `reader`."""

    @cfunc(pgf.expr_print)
    def print(self: Expr, writer: ~Writer, exn: Exn.Out) -> None:
        """Print an expression to `writer`."""

    @staticmethod
    def read_string(s, pool=None):
        """Read an expression from the string `s`."""
        with Pool() as p:
            r = Reader.new_string(s, p)
            return Expr.read(r, pool)

    def __str__(self):
        sio = io.StringIO()
        with Pool() as p:
            wtr = Writer.new(sio, p)
            self.print(wtr)
            wtr.flush()
        return sio.getvalue()

    class default_spec(util.instance(ProxySpec)):
        sot = cspec(Expr)
        def unwrap(x):
            if isinstance(x, str):
                return Expr.read_string(x)
            elif isinstance(x, Expr):
                return x
            raise TypeError


ExprEnum = enum(Expr)

class Parse(Abstract, metaclass=delay_init):
    """A parsing state."""
    
    @cfunc(pgf.parse_token)
    def token(self: ~dep(Parse), token: Token, pool: Pool.Out) -> ~Parse:
        """Feed a new token into the parse state. Returns a new state object."""

    @cfunc(pgf.parse_result)
    def result(self: ~Parse, pool: Pool.Out) -> ~ExprEnum:
        """Return an iterator over the :py:class:`Expr` trees that are
        completely parsed at the current parse state."""

class Parser(Abstract, metaclass=delay_init):
    @cfunc(pgf.new_parser, static=True)
    def new(concr: ~Concr, pool: Pool.Out) -> ~Parser:
        """Create a new :py:class:`Parser` for the grammar `concr`."""

    @cfunc(pgf.parser_parse)
    def parse(self: ~Parser, cat: ~Cat, cntn_id: CtntId, pool: Pool.Out
              ) -> ~Parse:
        """Begin parsing constituent `ctnt_id` of category `cat`.
        Returns a new :py:class`Parse` object representing the initial
        parsing state."""


class CncTree(Opaque):
    """A concrete syntax tree."""

CncTreeEnum = enum(CncTree)


Token = String
Tokens = Strings


class PresenterFuns(CStructure, delay=True):
    pass

class Presenter(CStructure):
    funs = Field(ref(PresenterFuns))

class _(PresenterFuns, metaclass=initialize):
    symbol_tokens = Field(fn(None, ~Presenter, Strings))
    _symbol_expr_dummy = Field(fn(None))
    _expr_apply_dummy = Field(fn(None))
    _expr_literal_dummy = Field(fn(None))
    _abort_dummy = Field(fn(None))
    _finish_dummy = Field(fn(None))

PresenterFuns.init()


class PresenterBridge(instance(BridgeSpec)):
    c_type = Presenter
    funs_type = PresenterFuns
    wrap_fields = ['symbol_tokens']

class Lzr(Abstract, metaclass=delay_init):
    """A linearizer."""

    @cfunc(pgf.new_lzr, static=True)
    def new(concr: ~Concr, pool: ~Pool.Out) -> ~Lzr:
        """Create a new linearizer for the concrete category `concr`."""

    @cfunc(pgf.lzr_concretize)
    def concretize(self: ~Lzr, expr: ~Expr, pool: ~Pool.Out) -> ~CncTreeEnum:
        """Iterate over the concrete trees corresponding to the abstract
        syntax tree `expr`."""

    @cfunc(pgf.lzr_linearize_simple)
    def linearize_simple(self: ~Lzr, tree: CncTree, id: CtntId,
                         out: ~Writer, exn: Exn.Out):
        """Linearize the concrete tree `tree` as space-separated tokens
        to the file-like object `out`."""

    @cfunc(pgf.lzr_linearize)
    def linearize(self: ~Lzr, tree: CncTree, id: CtntId,
                  presenter: ~PresenterBridge):
        """Linearize the concrete tree `tree` by sending presentation events
        to `presenter`."""
                  
