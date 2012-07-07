// Copyright 2012 University of Helsinki. Released under LGPL3.

#include <pgf/pgf.h>
#include <gu/log.h>
#include <gu/variant.h>
#include "data.h"


PgfConcr*
pgf_pgf_concr(PgfPGF* pgf, GuString cid, GuPool* pool)
{
	// The `pool` parameter may get used in the future, if concretes are
	// only loaded when needed.
	return gu_map_get(pgf->concretes, &cid, PgfConcr*);
}

PgfConcr*
pgf_pgf_concr_by_lang(PgfPGF* pgf, GuString lang, GuPool* opool)
{
	GuPool* pool = gu_local_pool();
	GuEnum* concrs = pgf_pgf_concrs(pgf, pool);
	PgfConcr* concr = NULL;
	PgfConcr* ret = NULL;
	while (gu_enum_next(concrs, &concr, NULL)) {
		GuString clang = pgf_concr_lang(concr);
		if (gu_string_eq(clang, lang)) {
			ret = concr;
			break;
		}
	}
	gu_pool_free(pool);
	return ret;
}


GuString
pgf_concr_id(PgfConcr* concr)
{
	return concr->id;
}

GuString
pgf_concr_lang(PgfConcr* concr)
{
	GuString ret = gu_null_string;
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

PgfCtntId
pgf_concr_cat_ctnt_id(PgfConcr* concr, PgfCat* cat, GuString ctnt)
{
	gu_require(cat->pgf == concr->pgf);
	PgfCncCat* cnccat = gu_map_get(concr->cnccats, cat, PgfCncCat*);
	if (!cnccat) {
		return PGF_CTNT_ID_BAD;
	}
	if (gu_string_is_null(ctnt)) {
		return cnccat->n_ctnts == 1 ? 0 : PGF_CTNT_ID_BAD;
	}
	return pgf_cnccat_ctnt_id_(cnccat, ctnt);
}

GuStrings
pgf_concr_cat_ctnts(PgfConcr* concr, PgfCat* cat, GuPool* pool)
{
	gu_require(cat->pgf == concr->pgf);
	PgfCncCat* cnccat = gu_map_get(concr->cnccats, cat, PgfCncCat*);
	if (!cnccat) {
		return gu_null_seq;
	}
	return GU_SEQ_COPY(cnccat->ctnts, GuString, pool);
}

GuEnum*
pgf_pgf_concrs(PgfPGF* pgf, GuPool* pool)
{
	return gu_map_values(pgf->concretes, pool);
}



PgfCat*
pgf_pgf_cat(PgfPGF* pgf, PgfCId cid)
{
	if (gu_string_is_null(cid)) {
		return pgf_pgf_startcat(pgf);
	}
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
