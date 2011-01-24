/* 
 * Copyright 2010 University of Helsinki.
 *   
 * This file is part of libpgf.
 * 
 * Libpgf is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * Libpgf is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libpgf. If not, see <http://www.gnu.org/licenses/>.
 */

#include <pgf/data.h>

typedef struct PgfLinFuncs PgfLinFuncs;

struct PgfLinFuncs {
	void (*symbol_tokens)(void* ctx, PgfTokens* toks);
	void (*symbol_expr)(void* ctx, int argno, PgfExpr expr, int lin_idx);

	void (*expr_apply)(void* ctx, PgfFunDecl* fun, int n_symbols);
	void (*expr_literal)(void* ctx, PgfLiteral lit);

	void (*abort)(void* ctx);
	void (*finish)(void* ctx);
};


typedef struct PgfLinearizer PgfLinearizer;

PgfLinearizer*
pgf_linearizer_new(GuPool* pool, PgfPGF* pgf, PgfConcr* cnc);


typedef struct PgfLinearization PgfLinearization;

PgfLinearization* pgf_lzn_new(GuPool* pool, PgfLinearizer* lzr);

bool
pgf_lzn_linearize_to_file(PgfLinearization* lzn, PgfExpr expr, PgfFId fid, int lin_idx, 
			  FILE* file_out);

