from libgu import *

pgf = Library('libpgf.so.0', "pgf_")

CId = String
Token = String
CtntId = c_int

class Cat(Abstract):
    pass

class Concr(Abstract):
    pass

Concr.id = pgf.concr_id(String, Concr)
Concr.lang = pgf.concr_lang(String, Concr)

ConcrEnum = enum(Concr)

class Pgf(Abstract):
    pass

Pgf.read = pgf.read_pgf.static(Pgf, In, Pool.Out, Exn.Out)
Pgf.startcat = pgf.pgf_startcat(Cat, Pgf)
Pgf.cat = pgf.pgf_cat(Cat, Pgf, CId)
Pgf.concr = pgf.pgf_concr(Concr, Pgf, CId, Pool.Out)
Pgf.concr_by_lang = pgf.pgf_concr_by_lang(Concr, Pgf, String, Pool.Out)
Pgf.concrs = pgf.pgf_concrs(ConcrEnum, Pgf, Pool.Out)

class Parse(Abstract):
    pass

Parse.token = pgf.parse_token(Parse, Parse, Token, Pool.Out)

class Parser(Abstract): 
    pass

Parser.new = pgf.new_parser.static(Parser, Concr, Pool.Out)
Parser.parse = pgf.parser_parse(Parse, Parser, Cat, CtntId, Pool.Out)

# XXX: reveal the variant structure
class Expr(Opaque):
    pass

ExprEnum = enum(Expr)


Parse.result = pgf.parse_result(ExprEnum, Parse, Pool.Out)
Parse.token = pgf.parse_token(Parse, Parse, Token, Pool.Out)

class CncTree(Opaque):
    pass

CncTreeEnum = enum(CncTree)

class Lzr(Abstract):
    pass

Lzr.new = pgf.new_lzr.static(Lzr, Concr, Pool.Out)
Lzr.concretize = pgf.lzr_concretize(CncTreeEnum, Lzr, Expr, Pool.Out)
Lzr.linearize_simple = pgf.lzr_linearize_simple(None, Lzr, CncTree, CtntId, 
                                                Writer, Exn.Out)




