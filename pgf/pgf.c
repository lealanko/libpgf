// Copyright 2012 University of Helsinki. Released under LGPL3.

#include <pgf/pgf.h>
#include <gu/log.h>
#include <gu/variant.h>
#include "pgf/data.h"


PgfConcr*
pgf_pgf_concr(PgfPGF* pgf, GuString cid, GuPool* pool)
{
	// The `pool` parameter may get used in the future, if concretes are
	// only loaded when needed.
	return gu_map_get(pgf->concretes, &cid, PgfConcr*);
}
	

GuString
pgf_concr_id(PgfConcr* concr)
{
	return concr->id;
}

GuString
pgf_concr_lang(PgfConcr* concr)
{
	GuString ret = gu_empty_string;
	GuPool* pool = gu_local_pool();
	GuString key = gu_str_string("language", pool);
	PgfLiteral lit = gu_map_get(concr->cflags, &key, PgfLiteral);
	if (gu_variant_is_null(lit)) {
		goto end;
	}
	GuVariantInfo i = gu_variant_open(lit);
	switch (i.tag) {
	case PGF_LITERAL_STR: {
		PgfLiteralStr* str = i.data;
		ret = str->val;
		break;
	}
	default:
		gu_warn("Malformed PGF? Flag \"language\" is not a string.");
		break;
	}
end:
	gu_pool_free(pool);
	return ret;
}

GuStrings
pgf_concr_cat_labels(PgfConcr* concr, PgfCat* cat, GuPool* pool)
{
	gu_require(cat->pgf == concr->pgf);
	PgfCncCat* cnccat = gu_map_get(concr->cnccats, &cat->cid, PgfCncCat*);
	if (!cnccat) {
		return gu_null_seq;
	}
	return GU_SEQ_COPY(cnccat->labels, GuString, pool);
}

GuEnum*
pgf_pgf_concrs(PgfPGF* pgf, GuPool* pool)
{
	return gu_map_values(pgf->concretes, pool);
}



PgfCat*
pgf_pgf_cat(PgfPGF* pgf, PgfCId cid)
{
	return gu_map_get(pgf->abstract.cats, &cid, PgfCat*);
}

PgfCat*
pgf_pgf_startcat(PgfPGF* pgf)
{
	GuPool* pool = gu_local_pool();
	GuString str = gu_str_string("startcat", pool);
	PgfLiteral lit = gu_map_get(pgf->abstract.aflags, &str, PgfLiteral);
	switch (gu_variant_tag(lit)) {
	case PGF_LITERAL_STR: {
		PgfLiteralStr* lits = gu_variant_data(lit);
		PgfCat* cat = pgf_pgf_cat(pgf, lits->val);
		if (!cat) {
			gu_warn("PGF named non-existent startcat");
		}
		return cat;
	}
	case GU_VARIANT_NULL:
		break;
	default:
		gu_warn("PGF had non-string startcat");
	}
	return NULL;
}
