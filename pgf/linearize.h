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


typedef struct PgfLinFuncs PgfLinFuncs;

struct PgfLinFuncs {
	void (*symbol_tokens)(PgfLinFuncs* funcs, PgfTokens* toks);
	void (*symbol_expr)(PgfLinFuncs* funcs, gint argno, PgfExpr expr, gint lin_idx);
	void (*expr_apply)(PgfLinFuncs* funcs,
			   PgfFunDecl* fun,
			   gint n_symbols);
	void (*expr_literal)(PgfLinFuncs* funcs, PgfLiteral lit);

	void (*abort)(PgfLinFuncs* funcs);
	void (*finish)(PgfLinFuncs* funcs);
};


