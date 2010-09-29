#ifndef PGF_H_
#define PGF_H_

#include <stdio.h>
#include <glib.h>

typedef struct PgfPGF PgfPGF;

PgfPGF* pgf_read(FILE* from, GError** err_out);

void pgf_write_yaml(PgfPGF* pgf, FILE* to, GError** err_out);

void pgf_free(PgfPGF* pgf);

#endif // PGF_H_

