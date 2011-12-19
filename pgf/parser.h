#ifndef PGF_PARSER_H_
#define PGF_PARSER_H_

#include <pgf/data.h>
#include <pgf/expr.h>

typedef struct PgfParser PgfParser;
typedef struct PgfParse PgfParse;
typedef struct PgfParseResult PgfParseResult;

PgfParser*
pgf_new_parser(PgfConcr* concr, GuPool* pool);

PgfParse*
pgf_parser_parse(PgfParser* parser, PgfCId cat, int lin_idx, GuPool* pool);

PgfParse*
pgf_parse_token(PgfParse* parse, PgfToken tok, GuPool* pool);

PgfParseResult*
pgf_parse_result(PgfParse* parse, GuPool* pool);

PgfExpr
pgf_parse_result_next(PgfParseResult* pr, GuPool* pool);



#endif // PGF_PARSER_H_
