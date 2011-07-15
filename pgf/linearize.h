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

#include <gu/type.h>
#include <gu/dump.h>
#include <pgf/data.h>

typedef struct PgfLzr PgfLzr;

GU_DECLARE_TYPE(PgfLzr, struct);

typedef struct PgfLzn PgfLzn;

typedef GuVariant PgfLinForm;

typedef struct PgfLinFuncs PgfLinFuncs;

struct PgfLinFuncs {
	void (*symbol_tokens)(PgfLinFuncs** self, PgfTokens* toks);
	void (*symbol_expr)(PgfLinFuncs** self, 
			    int argno, PgfExpr expr, int lin_idx);

	void (*expr_apply)(PgfLinFuncs** self, PgfCId* cid, int n_symbols);
	void (*expr_literal)(PgfLinFuncs** self, PgfLiteral lit);

	void (*abort)(PgfLinFuncs** self);
	void (*finish)(PgfLinFuncs** self);
};


PgfLzr*
pgf_lzr_new(GuPool* pool, PgfPGF* pgf, PgfConcr* cnc);

void
pgf_lzr_linearize(PgfLzr* lzr, PgfLinForm form, int lin_idx, PgfLinFuncs** fnsp);

int
pgf_lin_form_n_fields(PgfLinForm form, PgfLzr* lzr);

void
pgf_lzr_linearize_to_file(PgfLzr* lzr, PgfLinForm form, int lin_idx, 
			  FILE* file_out);

PgfLzn*
pgf_lzn_new(PgfLzr* lzr, PgfExpr expr, GuPool* pool);

PgfLinForm
pgf_lzn_next_form(PgfLzn* lzn, GuPool* pool);


extern GuTypeTable
pgf_linearize_dump_table;

