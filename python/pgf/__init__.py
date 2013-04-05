from gu import *

pgf = Library('libpgf.so.0', "pgf_")

CId = String
Token = String
CtntId = c_int

class Cat(Abstract):
    pass

class Concr(Abstract):
    pass

Concr.id = pgf.concr_id(String, ~Concr)
Concr.lang = pgf.concr_lang(String, ~Concr)

ConcrEnum = enum(~Concr)

class Pgf(Abstract):
    pass

Pgf.read = pgf.read_pgf.static(~Pgf, ~In, Pool.Out, Exn.Out)
Pgf.startcat = pgf.pgf_startcat(~Cat, dep(~Pgf))
Pgf.cat = pgf.pgf_cat(~Cat, dep(~Pgf), CId)
Pgf.concr = pgf.pgf_concr(~Concr, dep(~Pgf), CId, Pool.Out)
Pgf.concr_by_lang = pgf.pgf_concr_by_lang(~Concr, dep(~Pgf), String, Pool.Out)
Pgf.concrs = pgf.pgf_concrs(~ConcrEnum, dep(~Pgf), Pool.Out)

class Parse(Abstract):
    pass

Parse.token = pgf.parse_token(~Parse, ~Parse, Token, Pool.Out)

class Parser(Abstract): 
    pass

Parser.new = pgf.new_parser.static(~Parser, ~Concr, Pool.Out)
Parser.parse = pgf.parser_parse(~Parse, ~Parser, ~Cat, CtntId, Pool.Out)

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
