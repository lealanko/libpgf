from gu import *

pgf = Library('libpgf.so.0', "pgf_")

CId = String
Token = String
CtntId = c_int

class Cat(Abstract):
    pass

class Concr(Abstract, metaclass=delay_init):
    """A concrete grammar."""
    
    @cfunc(pgf.concr_id)
    def id(self: ~Concr) -> String:
        """Return the internal name of this concrete grammar."""

    @cfunc(pgf.concr_lang)
    def lang(self: ~Concr) -> String:
        """Return the language code for this concrete grammar."""

#Concr.id = pgf.concr_id(String, ~Concr)
#Concr.id.__doc__ = """Return the internal name of this concrete grammar."""
#Concr.lang = pgf.concr_lang(String, ~Concr)

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
    
    
class Parse(Abstract, metaclass=delay_init):
    @cfunc(pgf.parse_token)
    def token(self: ~dep(Parse), token: Token, pool: Pool.Out) -> ~Parse:
        """Feed a new token into the parse state."""

class Parser(Abstract, metaclass=delay_init):
    @cfunc(pgf.new_parser, static=True)
    def new(concr: ~Concr, pool: Pool.Out) -> ~Parser:
        """Create a new parser for the grammar `concr`."""

    @cfunc(pgf.parser_parse)
    def parse(self: ~Parser, cat: ~Cat, cntn_id: CtntId, pool: Pool.Out
              ) -> ~Parse:
        """Begin parsing constituent `ctnt_id` of category `cat`."""

class ReadExn(Abstract):
    pass

AbstractType.bind(pgf, 'PgfReadExn', ReadExn)

ExprType = Type.bind(pgf, 'PgfExpr')
Expr = ExprType.c_type

Expr.read = pgf.read_expr.static(Expr, ~Reader, Pool.Out, Exn.Out)
Expr.print = pgf.expr_print(None, Expr, ~Writer, Exn.Out)

def _expr_read_string(s, pool=None):
    with Pool() as p:
        r = Reader.new_string(s, p)
        return Expr.read(r, pool)

Expr.read_string = _expr_read_string

def _expr_str(expr):
    sio = io.StringIO()
    with Pool() as p:
        wtr = Writer.new(sio, p)
        expr.print(wtr)
        wtr.flush()
    return sio.getvalue()

Expr.__str__ = _expr_str


class _ExprSpec(util.instance(ProxySpec)):
    sot = cspec(Expr)
    def unwrap(x):
        if isinstance(x, str):
            return Expr.read_string(x)
        elif isinstance(x, Expr):
            return x
        raise TypeError

Expr.default_spec = _ExprSpec


ExprEnum = enum(Expr)


Parse.result = pgf.parse_result(~ExprEnum, ~Parse, Pool.Out)
Parse.token = pgf.parse_token(~Parse, ~Parse, Token, Pool.Out)

class CncTree(Opaque):
    pass

CncTreeEnum = enum(CncTree)


Token = String
Tokens = Strings



class PresenterFuns(CStructure, delay=True):
    pass

class Presenter(CStructure):
    funs = Field(ref(PresenterFuns))

PresenterFuns.symbol_tokens = Field(fn(None, ~Presenter, Strings))
PresenterFuns._symbol_expr_dummy = Field(fn(None))
PresenterFuns._expr_apply_dummy = Field(fn(None))
PresenterFuns._expr_literal_dummy = Field(fn(None))
PresenterFuns._abort_dummy = Field(fn(None))
PresenterFuns._finish_dummy = Field(fn(None))
PresenterFuns.init()


class PresenterBridge(instance(BridgeSpec)):
    c_type = Presenter
    funs_type = PresenterFuns
    wrap_fields = ['symbol_tokens']

class Lzr(Abstract):
    pass

Lzr.new = pgf.new_lzr.static(~Lzr, ~Concr, Pool.Out)
Lzr.concretize = pgf.lzr_concretize(~CncTreeEnum, ~Lzr, Expr, Pool.Out)
Lzr.linearize_simple = pgf.lzr_linearize_simple(None, ~Lzr, CncTree, CtntId, 
                                                ~Writer, Exn.Out)

Lzr.linearize = pgf.lzr_linearize(None, ~Lzr, CncTree, CtntId, ~PresenterBridge)
