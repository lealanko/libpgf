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

#ifndef PGF_H_
#define PGF_H_

#include <stdio.h>
#include <glib.h>

typedef struct PgfPGF PgfPGF;

PgfPGF* pgf_read(FILE* from, GError** err_out);

void pgf_write_yaml(PgfPGF* pgf, FILE* to, GError** err_out);

void pgf_free(PgfPGF* pgf);

#endif // PGF_H_
