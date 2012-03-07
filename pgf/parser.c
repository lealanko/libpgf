#include <pgf/parser.h>
#include <gu/choice.h>
#include <gu/seq.h>
#include <gu/assert.h>
#include <gu/log.h>
#include <gu/generic.h>
#include <gu/file.h>

typedef struct PgfItem PgfItem;

static GU_DECLARE_TYPE(PgfItem, struct);

enum {
	PGF_FID_SYNTHETIC = -999
};

typedef GuBuf PgfItemBuf;
typedef GuSeq PgfItemBufs;

static GU_DEFINE_TYPE(PgfItemBuf, GuBuf, gu_ptr_type(PgfItem));

typedef GuSet PgfItemSet; // -> PgfItem*
typedef GuSeq PgfItemSets;

static GU_DEFINE_TYPE(PgfItemSet, abstract, _);
// GuString -> PgfItemSet*			 
typedef GuMap PgfTransitions;			 

typedef GuBuf PgfCCatBuf;

struct PgfParser {
	PgfConcr* concr;
	GuHasher* item_hasher;
	GuHasher* tokens_hasher;
	GuWriter* dbg_wtr;
};

struct PgfParse {
	PgfParser* parser;
	PgfTransitions* transitions;
	PgfCCatBuf* completed;
};

typedef struct PgfParseResult PgfParseResult;

struct PgfParseResult {
	PgfCCatBuf* completed;
	GuChoice* choice;
	PgfExprEnum en;
};

typedef struct PgfItemBase PgfItemBase;

struct PgfItemBase {
	PgfItemSet* conts;
	PgfCCat* ccat;
	PgfProduction prod;
	GuWord lin_idx;
};

static GU_DEFINE_TYPE(
	PgfItemBase, struct,
	GU_MEMBER_P(PgfItemBase, conts, PgfItemSet),
	GU_MEMBER(PgfItemBase, ccat, PgfCCatId),
	GU_MEMBER(PgfItemBase, prod, PgfProduction),
	GU_MEMBER(PgfItemBase, lin_idx, GuWord));

struct PgfItem {
	PgfItemBase* base;
	PgfPArgs args;
	PgfSymbol curr_sym;
	uint16_t seq_idx;
	uint8_t tok_idx;
	uint8_t alt;
};

static GU_DEFINE_TYPE(
	PgfItem, struct,
	GU_MEMBER_P(PgfItem, base, PgfItemBase),
	GU_MEMBER(PgfItem, args, PgfPArgs),
	GU_MEMBER(PgfItem, curr_sym, PgfSymbol),
	GU_MEMBER(PgfItem, seq_idx, uint16_t),
	GU_MEMBER(PgfItem, tok_idx, uint8_t),
	GU_MEMBER(PgfItem, alt, uint8_t));

static void
pgf_symbol_print(PgfSymbol sym, size_t tok_idx, GuWriter* wtr, GuExn* exn)
{
	GuVariantInfo i = gu_variant_open(sym);
	if (tok_idx == 0) {
		gu_puts(". ", wtr, exn);
	}
	switch (i.tag) {
	case PGF_SYMBOL_KS: {
		PgfSymbolKS* sks = i.data;
		size_t n_toks = gu_seq_length(sks->tokens);
		PgfToken* toks = gu_seq_data(sks->tokens);
		for (size_t i = 0; i < n_toks; i++) {
			gu_string_write(toks[i], wtr, exn);
			gu_putc(' ', wtr, exn);
			if (i + 1 == tok_idx) {
				gu_puts(". ", wtr, exn);
			}
		}
		break;
	}
	case PGF_SYMBOL_CAT: {
		PgfSymbolCat* scat = i.data;
		gu_printf(wtr, exn, "<%d,%d> ", scat->d, scat->r);
		break;
	}
	default: {
		gu_puts("@ ", wtr, exn);
	}
	}
}

static void
pgf_ccat_print(PgfCCat* ccat, GuWriter* wtr, GuExn* exn)
{
	gu_printf(wtr, exn, "C%d(", ccat->fid);
	gu_string_write(ccat->cnccat->cid, wtr, exn);
	gu_puts(")", wtr, exn);
}

static void
pgf_cncfun_print(PgfCncFun* cncfun, GuWriter* wtr, GuExn* exn)
{
	gu_printf(wtr, exn, "F?("); // XXX: resolve fid
	gu_string_write(cncfun->fun, wtr, exn);
	gu_puts(")", wtr, exn);
}

static void
pgf_parg_print(PgfPArg* parg, GuWriter* wtr, GuExn* exn)
{
	pgf_ccat_print(parg->ccat, wtr, exn);
}

static void
pgf_item_print(PgfItem* item, GuWriter* wtr, GuExn* exn)
{
	gu_puts("[ ", wtr, exn);
	pgf_ccat_print(item->base->ccat, wtr, exn);
	gu_puts(" -> ", wtr, exn);
	GuVariantInfo i = gu_variant_open(item->base->prod);
	switch (i.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = i.data;
		pgf_cncfun_print(papp->fun, wtr, exn);
		gu_puts("[", wtr, exn);
		size_t n_pargs = gu_seq_length(papp->args);
		PgfPArg* pargs = gu_seq_data(papp->args);
		for (size_t i = 0; i < n_pargs; i++) {
			if (i > 0) {
				gu_puts(",", wtr, exn);
			}
			pgf_parg_print(&pargs[i], wtr, exn);
		}
		gu_printf(wtr, exn, "]; %u: ",
			  (unsigned) item->base->lin_idx);

		PgfSeqId sequence = gu_seq_get(papp->fun->lins,
					       PgfSeqId, item->base->lin_idx);
		size_t len = gu_seq_length(sequence);
		for (size_t n = 0; n < len; n++) {
			PgfSymbol sym = gu_seq_get(sequence, PgfSymbol, n);
			size_t tok_idx = ((item->seq_idx == n)
					  ? item->tok_idx : SIZE_MAX);
			pgf_symbol_print(sym, tok_idx, wtr, exn);
		}
		if (item->seq_idx == len) {
			gu_puts(". ", wtr, exn);
		}
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = i.data;
		gu_puts("_[", wtr, exn);
		pgf_ccat_print(pcoerce->coerce, wtr, exn);
		gu_printf(wtr, exn, "]; %u: ",
			  (unsigned) item->base->lin_idx);
		if (item->seq_idx == 0) {
			gu_puts(". <> ", wtr, exn);
		} else {
			gu_puts("<> . ", wtr, exn);
		}
		break;
	}
	default:
		gu_impossible();
	}
	gu_puts("]", wtr, exn);
}


typedef GuMap PgfContsMap;


static GU_DEFINE_TYPE(PgfItemSets, GuSeq, gu_ptr_type(PgfItemSet));

static GU_DEFINE_TYPE(PgfContsMap, GuMap,
		      gu_type(PgfCCat), NULL,
		      gu_type(PgfItemSets), &gu_null_seq);

static GU_DEFINE_TYPE(PgfGenCatMap, GuMap,
		      gu_type(PgfItemSet), NULL,
		      gu_ptr_type(PgfCCat), &gu_null_struct);

static GU_DEFINE_TYPE(PgfTransitions, GuStringMap,
		      gu_ptr_type(PgfItemSet), &gu_null_struct);

typedef GuMap PgfGenCatMap;

typedef struct PgfParsing PgfParsing;

struct PgfParsing {
	PgfParse* parse;
	GuPool* pool;
	PgfContsMap* conts_map;
	PgfGenCatMap* generated_cats;
};


static bool
pgf_tokens_equal(PgfParsing* parsing, PgfTokens toks1, PgfTokens toks2)
{
	return gu_eq(&parsing->parse->parser->tokens_hasher->eq,
		     &toks1, &toks2);
}

static void
pgf_parsing_add_transition(PgfParsing* parsing, PgfToken tok, PgfItem* item)
{
	// gu_debug("%s -> %p", tok, item);
	//gu_pdebug("", gu_string_printer, tok, " -> ", gu_ptr_printer, item, NULL);
	PgfTransitions* tmap = parsing->parse->transitions;
	PgfItemSet* items = gu_map_get(tmap, &tok, PgfItemSet*);
	if (!items) {
		items = gu_new_set(PgfItem*,
				   parsing->parse->parser->item_hasher,
				   parsing->pool);
		gu_map_put(tmap, &tok, PgfItemSet*, items);
	}
	gu_set_insert(items, &item);
}

static PgfItemSets
pgf_parsing_get_contss(PgfParsing* parsing, PgfCCat* cat)
{
	PgfItemSets contss = gu_map_get(parsing->conts_map, cat, PgfItemSets);
	if (gu_seq_is_null(contss)) {
		size_t n_lins = cat->cnccat->n_lins;
		contss = gu_new_seq(PgfItemSet*, n_lins, parsing->pool);
		for (size_t i = 0; i < n_lins; i++) {
			gu_seq_set(contss, PgfItemSet*, i, NULL);
		}
		gu_map_put(parsing->conts_map, cat, PgfItemSets, contss);
	}
	return contss;
}


static PgfItemSet*
pgf_parsing_get_conts(PgfParsing* parsing, PgfCCat* cat, size_t lin_idx)
{
	gu_require(lin_idx < cat->cnccat->n_lins);
	PgfItemSets contss = pgf_parsing_get_contss(parsing, cat);
	PgfItemSet* conts = gu_seq_get(contss, PgfItemSet*, lin_idx);
	if (!conts) {
		conts = gu_new_set(PgfItem*,
				   parsing->parse->parser->item_hasher,
				   parsing->pool);
		gu_seq_set(contss, PgfItemSet*, lin_idx, conts);
	}
	return conts;
}

static PgfCCat*
pgf_parsing_create_completed(PgfParsing* parsing, PgfItemSet* conts, 
			     PgfCncCat* cnccat)
{
	PgfCCat* cat = gu_new(PgfCCat, parsing->pool);
	cat->cnccat = cnccat;
	cat->fid = PGF_FID_SYNTHETIC;
	cat->prods = gu_buf_seq(gu_new_buf(PgfProduction, parsing->pool));
	gu_map_put(parsing->generated_cats, conts, PgfCCat*, cat);
	return cat;
}

static PgfCCat*
pgf_parsing_get_completed(PgfParsing* parsing, PgfItemSet* conts)
{
	return gu_map_get(parsing->generated_cats, conts, PgfCCat*);
}

static PgfSymbol
pgf_item_base_symbol(PgfItemBase* ibase, size_t seq_idx, GuPool* pool)
{
	GuVariantInfo i = gu_variant_open(ibase->prod);
	switch (i.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = i.data;
		PgfCncFun* fun = papp->fun;
		PgfSequence seq =
			gu_seq_get(fun->lins, PgfSequence, ibase->lin_idx);
		gu_assert(seq_idx <= gu_seq_length(seq));
		if (seq_idx == gu_seq_length(seq)) {
			return gu_null_variant;
		} else {
			return gu_seq_get(seq, PgfSymbol, seq_idx);
		}
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		gu_assert(seq_idx <= 1);
		if (seq_idx == 1) {
			return gu_null_variant;
		} else {
			return gu_new_variant_i(pool, PGF_SYMBOL_CAT,
						PgfSymbolCat,
						.d = 0, .r = ibase->lin_idx);
		}
		break;
	}
	default:
		gu_impossible();
	}
	return gu_null_variant;
}

static PgfItem*
pgf_new_item(PgfItemBase* base, GuPool* pool)
{
	PgfItem* item = gu_new(PgfItem, pool);
	GuVariantInfo pi = gu_variant_open(base->prod);
	switch (pi.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = pi.data;
		item->args = papp->args;
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = pi.data;
		item->args = gu_new_seq(PgfPArg, 1, pool);
		PgfPArg* parg = gu_seq_index(item->args, PgfPArg, 0);
		parg->hypos = gu_empty_seq();
		parg->ccat = pcoerce->coerce;
		break;
	}
	default:
		gu_impossible();
	}
	item->base = base;
	item->curr_sym = pgf_item_base_symbol(item->base, 0, pool);
	item->seq_idx = 0;
	item->tok_idx = 0;
	item->alt = 0;
	return item;
}

static PgfItem*
pgf_item_copy(PgfItem* item, GuPool* pool)
{
	PgfItem* copy = gu_new(PgfItem, pool);
	memcpy(copy, item, sizeof(PgfItem));
	return copy;
}

static void
pgf_item_advance(PgfItem* item, GuPool* pool)
{
	item->seq_idx++;
	item->curr_sym = pgf_item_base_symbol(item->base, item->seq_idx, pool);
}

static void
pgf_parsing_item(PgfParsing* parsing, PgfItem* item);

static void
pgf_parsing_combine(PgfParsing* parsing, PgfItem* cont, PgfCCat* cat)
{
	if (cont == NULL) {
		gu_buf_push(parsing->parse->completed, PgfCCat*, cat);
		return;
	}
	PgfItem* item = pgf_item_copy(cont, parsing->pool);
	size_t nargs = gu_seq_length(cont->args);
	item->args = gu_new_seq(PgfPArg, nargs, parsing->pool);
	memcpy(gu_seq_data(item->args), gu_seq_data(cont->args),
	       nargs * sizeof(PgfPArg));
	gu_assert(gu_variant_tag(item->curr_sym) == PGF_SYMBOL_CAT);
	PgfSymbolCat* pcat = gu_variant_data(cont->curr_sym);
	gu_seq_set(item->args, PgfPArg, pcat->d,
		   ((PgfPArg) { .hypos = gu_empty_seq(), .ccat = cat }));
	pgf_item_advance(item, parsing->pool);
	GuWriter* dwtr = parsing->parse->parser->dbg_wtr;
	if (dwtr) {
		gu_puts("combine: ", dwtr, NULL);
		pgf_item_print(item, dwtr, NULL);
		gu_puts("\n", dwtr, NULL);
	}
	pgf_parsing_item(parsing, item);
}

static void
pgf_parsing_production(PgfParsing* parsing, PgfCCat* cat, size_t lin_idx,
		       PgfProduction prod, PgfItemSet* conts)
{
	PgfItemBase* base = gu_new(PgfItemBase, parsing->pool);
	base->ccat = cat;
	base->lin_idx = lin_idx;
	base->prod = prod;
	base->conts = conts;
	PgfItem* item = pgf_new_item(base, parsing->pool);
	pgf_parsing_item(parsing, item);
}

static void
pgf_parsing_complete(PgfParsing* parsing, PgfItem* item)
{
	GuWriter* dwtr = parsing->parse->parser->dbg_wtr;
	if (dwtr) {
		gu_puts("complete: ", dwtr, NULL);
		pgf_item_print(item, dwtr, NULL);
		gu_puts("\n", dwtr, NULL);
	}
	GuVariantInfo i = gu_variant_open(item->base->prod);
	PgfProduction prod = gu_null_variant;
	switch (i.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = i.data;
		PgfProductionApply* new_papp = 
			gu_new_variant(PGF_PRODUCTION_APPLY,
				       PgfProductionApply,
				       &prod, parsing->pool);
		new_papp->fun = papp->fun;
		new_papp->args = item->args;
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* new_pcoerce =
			gu_new_variant(PGF_PRODUCTION_COERCE,
				       PgfProductionCoerce,
				       &prod, parsing->pool);
		PgfPArg* parg = gu_seq_index(item->args, PgfPArg, 0);
		gu_assert(gu_seq_is_empty(parg->hypos));
		new_pcoerce->coerce = parg->ccat;
		break;
	}
	default:
		gu_impossible();
	}
	PgfItemSet* conts = item->base->conts;
	PgfCCat* cat = pgf_parsing_get_completed(parsing, conts);
	if (cat != NULL) {
		// The category has already been created. If it has also been
		// predicted already, then process a new item for this
		// production.
		PgfItemSets contss = pgf_parsing_get_contss(parsing, cat);
		size_t n_contss = gu_seq_length(contss);
		PgfItemSet** contsd = gu_seq_data(contss);
		for (size_t i = 0; i < n_contss; i++) {
			PgfItemSet* conts2 = contsd[i];
			/* If there are continuations for
			 * linearization index i, then (cat, i) has
			 * already been predicted. Add the new
			 * production immediately to the agenda,
			 * i.e. process it. */
			if (conts2) {
				pgf_parsing_production(parsing, cat, i,
						       prod, conts2);
			}
		}
	} else {
		cat = pgf_parsing_create_completed(parsing, conts,
						   item->base->ccat->cnccat);
		GuPool* tmp_pool = gu_local_pool();
		GuEnum* cenum = gu_map_enum(conts, tmp_pool);
		PgfItem* cont;
		while (gu_enum_next(cenum, &cont, tmp_pool)) {
			pgf_parsing_combine(parsing, cont, cat);
		}
		gu_pool_free(tmp_pool);
	}
	GuBuf* prodbuf = gu_seq_buf(cat->prods);
	gu_buf_push(prodbuf, PgfProduction, prod);
}

static void
pgf_parsing_predict(PgfParsing* parsing, PgfItem* item, 
		    PgfCCat* cat, size_t lin_idx)
{
	gu_enter("-> cat: %d", cat->fid);
	if (gu_seq_is_null(cat->prods)) {
		// Empty category
		return;
	}
	PgfItemSet* conts = pgf_parsing_get_conts(parsing, cat, lin_idx);
	if (!gu_set_has(conts, &item)) {
		gu_set_insert(conts, &item);
		/* First time we encounter this linearization
		 * of this category at the current position,
		 * so predict it. */
		PgfProductions prods = cat->prods;
		size_t n_prods = gu_seq_length(prods);
		for (size_t i = 0; i < n_prods; i++) {
			PgfProduction prod =
				gu_seq_get(prods, PgfProduction, i);
			pgf_parsing_production(parsing, cat, lin_idx, 
					       prod, conts);
		}
	} //else {
		/* If it has already been completed, combine. */
		PgfCCat* completed = 
			pgf_parsing_get_completed(parsing, conts);
		if (completed) {
			pgf_parsing_combine(parsing, item, completed);
		}
		//}
	gu_exit(NULL);
}

static void
pgf_parsing_symbol(PgfParsing* parsing, PgfItem* item, PgfSymbol sym) {
	switch (gu_variant_tag(sym)) {
	case PGF_SYMBOL_CAT: {
		PgfSymbolCat* scat = gu_variant_data(sym);
		PgfPArg* parg = gu_seq_index(item->args, PgfPArg, scat->d);
		gu_assert(gu_seq_is_empty(parg->hypos));
		pgf_parsing_predict(parsing, item, parg->ccat, scat->r);
		break;
	}
	case PGF_SYMBOL_KS: {
		PgfSymbolKS* sks = gu_variant_data(sym);
		PgfToken tok = 
			gu_seq_get(sks->tokens, PgfToken, item->tok_idx);
		pgf_parsing_add_transition(parsing, tok, item);
		break;
	}
	case PGF_SYMBOL_KP: {
		PgfSymbolKP* skp = gu_variant_data(sym);
		size_t idx = item->tok_idx;
		uint8_t alt = item->alt;
		size_t n_alts = gu_seq_length(skp->alts);
		PgfAlternative* alts = gu_seq_data(skp->alts);
		if (idx == 0) {
			PgfToken tok = gu_seq_get(skp->default_form, PgfToken, 0);
			pgf_parsing_add_transition(parsing, tok, item);
			for (size_t i = 0; i < n_alts; i++) {
				PgfTokens toks = alts[i].form;
				PgfTokens toks2 = skp->default_form;
				// XXX: do nubbing properly
				bool skip = pgf_tokens_equal(parsing, toks, toks2);
				for (size_t j = 0; j < i; j++) {
					PgfTokens toks2 = alts[j].form;
					skip |= pgf_tokens_equal(parsing, toks, toks2);
				}
				if (skip) {
					continue;
				}
				PgfToken tok = gu_seq_get(toks, PgfToken, 0);
				pgf_parsing_add_transition(parsing, tok, item);
			}
		} else if (alt == 0) {
			PgfToken tok = 
				gu_seq_get(skp->default_form, PgfToken, idx);
			pgf_parsing_add_transition(parsing, tok, item);
		} else {
			gu_assert(alt <= n_alts);
			PgfToken tok = gu_seq_get(alts[alt - 1].form, 
						  PgfToken, idx);
			pgf_parsing_add_transition(parsing, tok, item);
		}
		break;
	}
	case PGF_SYMBOL_LIT:
		// XXX TODO proper support
		break;
	case PGF_SYMBOL_VAR:
		// XXX TODO proper support
		break;
	default:
		gu_impossible();
	}
}

static void
pgf_parsing_item(PgfParsing* parsing, PgfItem* item)
{
	GuVariantInfo i = gu_variant_open(item->base->prod);
	switch (i.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = i.data;
		PgfCncFun* fun = papp->fun;
		PgfSequence seq = gu_seq_get(fun->lins, PgfSequence,
					     item->base->lin_idx);
		if (item->seq_idx == gu_seq_length(seq)) {
			pgf_parsing_complete(parsing, item);
		} else  {
			PgfSymbol sym = 
				gu_seq_get(seq, PgfSymbol, item->seq_idx);
			pgf_parsing_symbol(parsing, item, sym);
		}
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = i.data;
		switch (item->seq_idx) {
		case 0:
			pgf_parsing_predict(parsing, item, 
					    pcoerce->coerce,
					    item->base->lin_idx);
			break;
		case 1:
			pgf_parsing_complete(parsing, item);
			break;
		default:
			gu_impossible();
		}
		break;
	}
	default:
		gu_impossible();
	}
}

static bool
pgf_parsing_scan_toks(PgfParsing* parsing, PgfItem* old_item, 
		      PgfToken tok, int alt, PgfTokens toks)
{
	if (!gu_string_eq(gu_seq_get(toks, PgfToken, old_item->tok_idx), 
			  tok)) {
		return false;
	}
	PgfItem* item = pgf_item_copy(old_item, parsing->pool);
	item->tok_idx++;
	item->alt = alt;
	if (item->tok_idx == gu_seq_length(toks)) {
		item->tok_idx = 0;
		pgf_item_advance(item, parsing->pool);
	}
	pgf_parsing_item(parsing, item);
	return true;
}

static void
pgf_parsing_scan(PgfParsing* parsing, PgfItem* item, PgfToken tok)
{
	bool succ = false;
	GuWriter* dwtr = parsing->parse->parser->dbg_wtr;
	if (dwtr) {
		pgf_item_print(item, dwtr, NULL);
		gu_puts("\n", dwtr, NULL);
	}
	GuVariantInfo i = gu_variant_open(item->curr_sym);
	switch (i.tag) {
	case PGF_SYMBOL_KS: {
		PgfSymbolKS* ks = i.data;
		succ = pgf_parsing_scan_toks(parsing, item, tok, 0, 
					     ks->tokens);
		break;
	}
	case PGF_SYMBOL_KP: {
		PgfSymbolKP* kp = i.data;
		size_t alt = item->alt;
		size_t n_alts = gu_seq_length(kp->alts);
		PgfAlternative* alts = gu_seq_data(kp->alts);
		if (item->tok_idx == 0) {
			succ = pgf_parsing_scan_toks(parsing, item, tok, 0, 
						      kp->default_form);
			for (size_t i = 0; i < n_alts; i++) {
				// XXX: do nubbing properly
				PgfTokens toks = alts[i].form;
				PgfTokens toks2 = kp->default_form;
				bool skip = pgf_tokens_equal(parsing, toks, toks2);
				for (size_t j = 0; j < i; j++) {
					PgfTokens toks2 = alts[j].form;
					skip |= pgf_tokens_equal(parsing, toks, toks2);
				}
				if (!skip) {
					succ |= pgf_parsing_scan_toks(
						parsing, item, tok, i + 1,
						alts[i].form);
				}
			}
		} else if (alt == 0) {
			succ = pgf_parsing_scan_toks(parsing, item, tok, 0, 
						      kp->default_form);
		} else {
			gu_assert(alt <= n_alts);
			succ = pgf_parsing_scan_toks(parsing, item, tok, 
						     alt, alts[alt - 1].form);
		}
		break;
	}
	default:
		gu_impossible();
	}
	gu_assert(succ);
}


static PgfParsing*
pgf_new_parsing(PgfParse* parse, GuPool* parse_pool, GuPool* out_pool)
{
	PgfParsing* parsing = gu_new(PgfParsing, out_pool);
	parsing->parse = parse;
	parsing->generated_cats = gu_map_type_new(PgfGenCatMap, out_pool);
	parsing->conts_map = gu_map_type_new(PgfContsMap, out_pool);
	parsing->pool = parse_pool;
	return parsing;
}

static PgfParse*
pgf_new_parse(PgfParser* parser, GuPool* pool)
{
	PgfParse* parse = gu_new(PgfParse, pool);
	parse->parser = parser;
	parse->transitions = gu_map_type_new(PgfTransitions, pool);
	parse->completed = gu_new_buf(PgfCCat*, pool);
	return parse;
}

PgfParse*
pgf_parse_token(PgfParse* parse, PgfToken tok, GuPool* pool)
{
	PgfItemSet* agenda =
		gu_map_get(parse->transitions, &tok, PgfItemSet*);
	if (!agenda) {
		return NULL;
	}
	GuWriter* dwtr = parse->parser->dbg_wtr;
	if (dwtr) {
		gu_puts("scan: ", dwtr, NULL);
		gu_string_write(tok, dwtr, NULL);
		gu_puts("\n", dwtr, NULL);
	}
	PgfParse* next_parse = pgf_new_parse(parse->parser, pool);
	GuPool* tmp_pool = gu_new_pool();
	PgfParsing* parsing = pgf_new_parsing(next_parse, pool, tmp_pool);
	GuEnum* items = gu_map_enum(agenda, tmp_pool);
	PgfItem* item;
	while (gu_enum_next(items, &item, tmp_pool)) {
		pgf_parsing_scan(parsing, item, tok);
	}
	gu_pool_free(tmp_pool);
	return next_parse;
}

static PgfExpr
pgf_cat_to_expr(PgfCCat* cat, GuChoice* choice, GuSet* seen_cats, GuPool* pool);

static PgfExpr
pgf_production_to_expr(PgfProduction prod, GuChoice* choice,
		       GuSet* seen_cats, GuPool* pool)
{
	GuVariantInfo pi = gu_variant_open(prod);
	switch (pi.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = pi.data;
		PgfExpr expr = gu_new_variant_i(pool, PGF_EXPR_FUN,
						PgfExprFun,
						.fun = papp->fun->fun);
		size_t n_args = gu_seq_length(papp->args);
		for (size_t i = 0; i < n_args; i++) {
			PgfPArg* parg = gu_seq_index(papp->args, PgfPArg, i);
			gu_assert(gu_seq_is_empty(parg->hypos));
			PgfExpr earg = pgf_cat_to_expr(parg->ccat, choice,
						       seen_cats, pool);
			if (gu_variant_is_null(earg)) {
				return gu_null_variant;
			}
			expr = gu_new_variant_i(pool, PGF_EXPR_APP,
						PgfExprApp,
						.fun = expr, .arg = earg);
		}
		return expr;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = pi.data;
		return pgf_cat_to_expr(pcoerce->coerce, choice,
				       seen_cats, pool);
	}
	default:
		gu_impossible();
	}
	return gu_null_variant;
}


static PgfExpr
pgf_cat_to_expr(PgfCCat* cat, GuChoice* choice,
		GuSet* seen_cats, GuPool* pool)
{
	if (gu_set_has(seen_cats, cat)) {
		// cyclic expression
		return gu_null_variant;
	}
	gu_set_insert(seen_cats, cat);
	if (cat->fid != PGF_FID_SYNTHETIC) {
		// XXX: What should the PgfMetaId be?
		return gu_new_variant_i(pool, PGF_EXPR_META,
					PgfExprMeta, 
					.id = 0);
	}
	size_t n_prods = gu_seq_length(cat->prods);
	int i = gu_choice_next(choice, n_prods);
	if (i == -1) {
		return gu_null_variant;
	}
	PgfProduction prod = gu_seq_get(cat->prods, PgfProduction, i);
	return pgf_production_to_expr(prod, choice, seen_cats, pool);
}


static PgfExpr
pgf_parse_result_next(PgfParseResult* pr, GuPool* pool)
{
	if (pr->choice == NULL) {
		return gu_null_variant;
	}
	size_t n_results = gu_buf_length(pr->completed);
	PgfExpr ret = gu_null_variant;
	while (gu_variant_is_null(ret)) {
		GuChoiceMark mark = gu_choice_mark(pr->choice);
		int i = gu_choice_next(pr->choice, n_results);
		if (i == -1) {
			return gu_null_variant;
		}
		PgfCCat* cat = gu_buf_get(pr->completed, PgfCCat*, i);
		GuPool* tmp_pool = gu_new_pool();
		GuSet* seen_cats = gu_new_addr_set(PgfCCat, tmp_pool);
		ret = pgf_cat_to_expr(cat, pr->choice, seen_cats, pool);
		gu_pool_free(tmp_pool);
		gu_choice_reset(pr->choice, mark);
		if (!gu_choice_advance(pr->choice)) {
			pr->choice = NULL;
		};
	}
	return ret;
}

static bool
pgf_parse_result_enum_next(GuEnum* self, void* to, GuPool* pool)
{
	PgfParseResult* pr = gu_container(self, PgfParseResult, en);
	PgfExpr expr = pgf_parse_result_next(pr, pool);
	if (gu_variant_is_null(expr)) {
		return false;
	}
	*(PgfExpr*)to = expr;
	return true;
}

PgfExprEnum*
pgf_parse_result(PgfParse* parse, GuPool* pool)
{
	return &gu_new_i(pool, PgfParseResult,
			 .completed = parse->completed,
			 .choice = gu_new_choice(pool),
			 .en.next = pgf_parse_result_enum_next)->en;
}



// TODO: s/CId/Cat, add the cid to Cat, make Cat the key to CncCat
PgfParse*
pgf_parser_parse(PgfParser* parser, PgfCId cat, size_t lin_idx, GuPool* pool)
{
	PgfParse* parse = pgf_new_parse(parser, pool);
	GuPool* tmp_pool = gu_new_pool();
	PgfParsing* parsing = pgf_new_parsing(parse, pool, tmp_pool);
	PgfCncCat* cnccat =
		gu_map_get(parser->concr->cnccats, &cat, PgfCncCat*);
	if (!cnccat) {
		// error ...
		gu_impossible();
	}
	gu_assert(lin_idx < cnccat->n_lins);
	size_t n_ccats = gu_seq_length(cnccat->cats);
	for (size_t i = 0; i < n_ccats; i++) {
		PgfCCat* ccat = gu_seq_get(cnccat->cats, PgfCCat*, i);
		if (ccat != NULL) {
			pgf_parsing_predict(parsing, NULL, ccat, lin_idx);
		}
	}
	gu_pool_free(tmp_pool);
	return parse;
}

PgfParser* 
pgf_new_parser(PgfConcr* concr, GuPool* pool)
{
	gu_require(concr != NULL);
	PgfParser* parser = gu_new(PgfParser, pool);
	parser->concr = concr;
	GuPool* tmp_pool = gu_local_pool();
	GuGeneric* gen_hasher =
		gu_new_generic(gu_hasher_instances, tmp_pool);
	parser->item_hasher =
		gu_specialize(gen_hasher, gu_ptr_type(PgfItem), pool);
	parser->tokens_hasher =
		gu_specialize(gen_hasher, gu_type(PgfTokens), pool);
	parser->dbg_wtr =
		gu_new_utf8_writer(gu_file_out(stderr, pool), pool);
	gu_pool_free(tmp_pool);
	return parser;
}
