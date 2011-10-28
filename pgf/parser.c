#include <pgf/data.h>
#include <pgf/expr.h>
#include <gu/choice.h>
#include <gu/seq.h>
#include <gu/assert.h>
#include <gu/log.h>

typedef struct PgfItem PgfItem;

GU_SEQ_DEFINE(PgfItems, pgf_items, PgfItem*);

enum {
	PGF_FID_SYNTHETIC = -999
};

typedef GuList(PgfItems) PgfItemss;

GU_STRMAP_DEFINE(PgfTransitions, pgf_transitions,
		 V, PgfItems, GU_SEQ_NULL);
			   
typedef struct PgfParser PgfParser;

struct PgfParser {
	PgfConcr* concr;
};

typedef struct PgfParse PgfParse;

struct PgfParse {
	PgfParser* parser;
	PgfTransitions* transitions;
	PgfCCatSeq completed;
};

typedef struct PgfParseResult PgfParseResult;

struct PgfParseResult {
	PgfCCatSeq completed;
	GuChoice* choice;
};

typedef struct PgfItemBase PgfItemBase;

struct PgfItemBase {
	PgfItems conts;
	PgfCCat* ccat;
	PgfProduction prod;
	short lin_idx;
};

struct PgfItem {
	PgfItemBase* base;
	PgfPArgs* args;
	PgfSymbol curr_sym;
	uint16_t seq_idx;
	uint8_t tok_idx;
	uint8_t alt;
};

//typedef GuPtrMap PgfContsMap; // PgfCCat* -> PgfItemss*
GU_PTRMAP_DEFINE(PgfContsMap, pgf_conts_map, PgfCCat, R, PgfItemss, NULL);

//typedef GuPtrMap PgfGenCatMap; // PgfConts* |-> PgfCCat*
//GU_PTRMAP_DEFINE(PgfGenCatMap, pgf_gencat_map, PgfConts, R, PgfCCat, NULL);
GU_MAP_DEFINE(PgfGenCatMap, pgf_gencat_map, V, PgfItems, R, PgfCCat, NULL, NULL, NULL);


typedef struct PgfParsing PgfParsing;

struct PgfParsing {
	PgfParse* parse;
	GuPool* pool;
	PgfContsMap* conts_map;
	PgfGenCatMap* generated_cats;
};

static void
pgf_parsing_add_transition(PgfParsing* parsing, PgfToken tok, PgfItem* item)
{
	gu_debug("%s -> %p", tok, item);
	PgfTransitions* tmap = parsing->parse->transitions;
	PgfItems items = pgf_transitions_get(tmap, tok);
	if (pgf_items_is_null(items)) {
		items = pgf_items_new(parsing->pool);
		pgf_transitions_set(tmap, tok, items);
	}
	pgf_items_push(items, item);
}

static PgfItemss*
pgf_parsing_get_contss(PgfParsing* parsing, PgfCCat* cat)
{
	PgfItemss* contss = pgf_conts_map_get(parsing->conts_map, cat);
	if (!contss) {
		int n_lins = cat->cnccat->n_lins;
		contss = gu_list_new(PgfItemss, parsing->pool, n_lins);
		for (int i = 0; i < n_lins; i++) {
			gu_list_index(contss, i) = (PgfItems){ GU_SEQ_NULL };
		}
		pgf_conts_map_set(parsing->conts_map, cat, contss);
	}
	return contss;
}


static PgfItems
pgf_parsing_get_conts(PgfParsing* parsing, PgfCCat* cat, int lin_idx)
{
	gu_require(lin_idx < cat->cnccat->n_lins);
	PgfItemss* contss = pgf_parsing_get_contss(parsing, cat);
	PgfItems conts = gu_list_index(contss, lin_idx);
	if (pgf_items_is_null(conts)) {
		conts = pgf_items_new(parsing->pool);
		gu_list_index(contss, lin_idx) = conts;
	}
	return conts;
}

static PgfCCat*
pgf_parsing_create_completed(PgfParsing* parsing, PgfItems conts, 
			     PgfCncCat* cnccat)
{
	PgfCCat* cat = gu_new(parsing->pool, PgfCCat);
	cat->cnccat = cnccat;
	cat->fid = PGF_FID_SYNTHETIC;
	cat->prods = pgf_production_seq_new(parsing->pool);
	pgf_gencat_map_set(parsing->generated_cats, conts, cat);
	return cat;
}

static PgfCCat*
pgf_parsing_get_completed(PgfParsing* parsing, PgfItems conts)
{
	return pgf_gencat_map_get(parsing->generated_cats, conts);
}

static PgfSymbol
pgf_item_base_symbol(PgfItemBase* ibase, int seq_idx, GuPool* pool)
{
	GuVariantInfo i = gu_variant_open(ibase->prod);
	switch (i.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = i.data;
		PgfCncFun* fun = papp->fun;
		gu_assert(ibase->lin_idx < fun->n_lins);
		PgfSequence* seq = fun->lins[ibase->lin_idx];
		gu_assert(seq_idx <= gu_list_length(seq));
		if (seq_idx == gu_list_length(seq)) {
			return gu_variant_null;
		} else {
			return gu_list_index(seq, seq_idx);
		}
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		gu_assert(seq_idx <= 1);
		if (seq_idx == 1) {
			return gu_variant_null;
		} else {
			return gu_variant_new_i(pool, PGF_SYMBOL_CAT,
						PgfSymbolCat,
						.d = 0, .r = ibase->lin_idx);
		}
		break;
	}
	default:
		gu_impossible();
	}
	return gu_variant_null;
}

static PgfItem*
pgf_item_new(PgfItemBase* base, GuPool* pool)
{
	PgfItem* item = gu_new(pool, PgfItem);
	GuVariantInfo pi = gu_variant_open(base->prod);
	switch (pi.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = pi.data;
		item->args = papp->args;
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = pi.data;
		item->args = gu_list_new(PgfPArgs, pool, 1);
		gu_list_index(item->args, 0) = 
			(PgfPArg) { .hypos = NULL, .ccat = pcoerce->coerce };
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
	PgfItem* copy = gu_new(pool, PgfItem);
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
		pgf_ccat_seq_push(parsing->parse->completed, cat);
		return;
	}
	PgfItem* item = pgf_item_copy(cont, parsing->pool);
	int nargs = gu_list_length(cont->args);
	item->args = gu_list_new(PgfPArgs, parsing->pool, nargs);
	memcpy(gu_list_elems(item->args), gu_list_elems(cont->args),
	       nargs * sizeof(PgfPArg));
	gu_assert(gu_variant_tag(item->curr_sym) == PGF_SYMBOL_CAT);
	PgfSymbolCat* pcat = gu_variant_data(cont->curr_sym);
	gu_list_index(item->args, pcat->d) =
		(PgfPArg) { .hypos = NULL, .ccat = cat };
	pgf_item_advance(item, parsing->pool);
	pgf_parsing_item(parsing, item);
}

static void
pgf_parsing_production(PgfParsing* parsing, PgfCCat* cat, int lin_idx,
		       PgfProduction prod, PgfItems conts)
{
	PgfItemBase* base = gu_new(parsing->pool, PgfItemBase);
	base->ccat = cat;
	base->lin_idx = lin_idx;
	base->prod = prod;
	base->conts = conts;
	PgfItem* item = pgf_item_new(base, parsing->pool);
	pgf_parsing_item(parsing, item);
}

static void
pgf_parsing_complete(PgfParsing* parsing, PgfItem* item)
{
	GuVariantInfo i = gu_variant_open(item->base->prod);
	PgfProduction prod = gu_variant_null;
	switch (i.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = i.data;
		PgfProductionApply* new_papp = 
			gu_variant_new(parsing->pool,
				       PGF_PRODUCTION_APPLY,
				       PgfProductionApply,
				       &prod);
		new_papp->fun = papp->fun;
		new_papp->args = item->args;
		break;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* new_pcoerce =
			gu_variant_new(parsing->pool,
				       PGF_PRODUCTION_COERCE,
				       PgfProductionCoerce,
				       &prod);
		PgfPArg* parg = &gu_list_index(item->args, 0);
		gu_assert(!parg->hypos || !parg->hypos->len);
		new_pcoerce->coerce = parg->ccat;
		break;
	}
	default:
		gu_impossible();
	}
	PgfItems conts = item->base->conts;
	PgfCCat* cat = pgf_parsing_get_completed(parsing, conts);
	if (cat != NULL) {
		// The category has already been created. If it has also been
		// predicted already, then process a new item for this production.
		PgfItemss* contss = pgf_parsing_get_contss(parsing, cat);
		int n_contss = gu_list_length(contss);
		for (int i = 0; i < n_contss; i++) {
			PgfItems conts2 = gu_list_index(contss, i);
			/* If there are continuations for
			 * linearization index i, then (cat, i) has
			 * already been predicted. Add the new
			 * production immediately to the agenda,
			 * i.e. process it. */
			if (!pgf_items_is_null(conts2)) {
				pgf_parsing_production(parsing, cat, i,
						       prod, conts2);
			}
		}
	} else {
		cat = pgf_parsing_create_completed(parsing, conts,
						   item->base->ccat->cnccat);
		int n_conts = pgf_items_size(conts);
		for (int i = 0; i < n_conts; i++) {
			PgfItem* cont = pgf_items_get(conts, i);
			pgf_parsing_combine(parsing, cont, cat);
		}
	}
	pgf_production_seq_push(cat->prods, prod);
}


static void
pgf_parsing_predict(PgfParsing* parsing, PgfItem* item, 
		    PgfCCat* cat, int lin_idx)
{
	gu_enter("-> cat: %d", cat->fid);
	if (pgf_production_seq_is_null(cat->prods)) {
		// Empty category
		return;
	}
	PgfItems conts = pgf_parsing_get_conts(parsing, cat, lin_idx);
	pgf_items_push(conts, item);
	if (pgf_items_size(conts) == 1) {
		/* First time we encounter this linearization
		 * of this category at the current position,
		 * so predict it. */
		PgfProductionSeq prods = cat->prods;
		int n_prods = pgf_production_seq_size(prods);
		for (int i = 0; i < n_prods; i++) {
			PgfProduction prod = pgf_production_seq_get(prods, i);
			pgf_parsing_production(parsing, cat, lin_idx, 
					       prod, conts);
		}
	} else {
		/* If it has already been completed, combine. */
		PgfCCat* completed = 
			pgf_parsing_get_completed(parsing, conts);
		if (completed) {
			pgf_parsing_combine(parsing, item, completed);
		}
	}
	gu_exit(NULL);
}

static void
pgf_parsing_symbol(PgfParsing* parsing, PgfItem* item, PgfSymbol sym) {
	switch (gu_variant_tag(sym)) {
	case PGF_SYMBOL_CAT: {
		PgfSymbolCat* scat = gu_variant_data(sym);
		PgfPArg* parg = &gu_list_index(item->args, scat->d);
		gu_assert(!parg->hypos || !parg->hypos->len);
		pgf_parsing_predict(parsing, item, parg->ccat, scat->r);
		break;;
	}
	case PGF_SYMBOL_KS: {
		PgfSymbolKS* sks = gu_variant_data(sym);
		gu_assert(item->tok_idx < gu_list_length(sks->tokens));
		PgfToken tok = gu_list_index(sks->tokens, item->tok_idx);
		pgf_parsing_add_transition(parsing, tok, item);
		break;
	}
	case PGF_SYMBOL_KP: {
		PgfSymbolKP* skp = gu_variant_data(sym);
		int idx = item->tok_idx;
		int alt = item->alt;
		gu_assert(idx < gu_list_length(skp->default_form));
		if (idx == 0) {
			PgfToken tok = gu_list_index(skp->default_form, 0);
			pgf_parsing_add_transition(parsing, tok, item);
			for (int i = 0; i < skp->n_forms; i++) {
				PgfTokens* toks = skp->forms[i].form;
				PgfTokens* toks2 = skp->default_form;
				bool skip = pgf_tokens_equal(toks, toks2);
				for (int j = 0; j < i; j++) {
					PgfTokens* toks2 = skp->forms[j].form;
					skip |= pgf_tokens_equal(toks, toks2);
				}
				if (skip) {
					continue;
				}
				PgfToken tok = gu_list_index(toks, 0);
				pgf_parsing_add_transition(parsing, tok, item);
			}
		} else if (alt == 0) {
			PgfToken tok = gu_list_index(skp->default_form, idx);
			pgf_parsing_add_transition(parsing, tok, item);
		} else {
			gu_assert(alt <= skp->n_forms);
			PgfToken tok = gu_list_index(skp->forms[alt - 1].form, 
						     idx);
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
		PgfSequence* seq = fun->lins[item->base->lin_idx];
		if (item->seq_idx == gu_list_length(seq)) {
			pgf_parsing_complete(parsing, item);
		} else  {
			PgfSymbol sym = gu_list_index(seq, item->seq_idx);
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
		      PgfToken tok, int alt, PgfTokens* toks)
{
	gu_assert(old_item->tok_idx < gu_list_length(toks));
	if (strcmp(gu_list_index(toks, old_item->tok_idx), tok) != 0) {
		return false;
	}
	PgfItem* item = pgf_item_copy(old_item, parsing->pool);
	item->tok_idx++;
	item->alt = alt;
	if (item->tok_idx == gu_list_length(toks)) {
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
		int alt = item->alt;
		if (item->tok_idx == 0) {
			succ = pgf_parsing_scan_toks(parsing, item, tok, 0, 
						      kp->default_form);
			for (int i = 0; i < kp->n_forms; i++) {
				PgfTokens* toks = kp->forms[i].form;
				PgfTokens* toks2 = kp->default_form;
				bool skip = pgf_tokens_equal(toks, toks2);
				for (int j = 0; j < i; j++) {
					PgfTokens* toks2 = kp->forms[j].form;
					skip |= pgf_tokens_equal(toks, toks2);
				}
				if (!skip) {
					succ |= pgf_parsing_scan_toks(
						parsing, item, tok, i + 1,
						kp->forms[i].form);
				}
			}
		} else if (alt == 0) {
			succ = pgf_parsing_scan_toks(parsing, item, tok, 0, 
						      kp->default_form);
		} else {
			gu_assert(alt <= kp->n_forms);
			succ = pgf_parsing_scan_toks(parsing, item, tok, 
						     alt, kp->forms[alt - 1].form);
		}
		break;
	}
	default:
		gu_impossible();
	}
	gu_assert(succ);
}


static PgfParsing*
pgf_parsing_new(PgfParse* parse, GuPool* parse_pool, GuPool* out_pool)
{
	PgfParsing* parsing = gu_new(out_pool, PgfParsing);
	parsing->parse = parse;
	parsing->generated_cats = pgf_gencat_map_new(out_pool);
	parsing->conts_map = pgf_conts_map_new(out_pool);
	parsing->pool = parse_pool;
	return parsing;
}

static PgfParse*
pgf_parse_new(PgfParser* parser, GuPool* pool)
{
	PgfParse* parse = gu_new(pool, PgfParse);
	parse->parser = parser;
	parse->transitions = pgf_transitions_new(pool);
	parse->completed = pgf_ccat_seq_new(pool);
	return parse;
}

PgfParse*
pgf_parse_token(PgfParse* parse, PgfToken tok, GuPool* pool)
{
	PgfItems agenda = pgf_transitions_get(parse->transitions, tok);
	if (pgf_items_is_null(agenda)) {
		return NULL;
	}
	PgfParse* next_parse = pgf_parse_new(parse->parser, pool);
	GuPool* tmp_pool = gu_pool_new();
	PgfParsing* parsing = pgf_parsing_new(next_parse, pool, tmp_pool);
	int n_items = pgf_items_size(agenda);
	for (int i = 0; i < n_items; i++) {
		PgfItem* item = pgf_items_get(agenda, i);
		pgf_parsing_scan(parsing, item, tok);
	}
	gu_pool_free(tmp_pool);
	return next_parse;
}

static PgfExpr
pgf_cat_to_expr(PgfCCat* cat, GuChoice* choice, GuPool* pool);

static PgfExpr
pgf_production_to_expr(PgfProduction prod, GuChoice* choice, GuPool* pool)
{
	GuVariantInfo pi = gu_variant_open(prod);
	switch (pi.tag) {
	case PGF_PRODUCTION_APPLY: {
		PgfProductionApply* papp = pi.data;
		PgfExpr expr = gu_variant_new_i(pool, PGF_EXPR_FUN,
						PgfExprFun,
						.fun = papp->fun->fun);
		int n_args = gu_list_length(papp->args);
		for (int i = 0; i < n_args; i++) {
			PgfPArg* parg = &gu_list_index(papp->args, i);
			gu_assert(!parg->hypos || !parg->hypos->len);
			PgfExpr earg = pgf_cat_to_expr(parg->ccat, choice, pool);
			expr = gu_variant_new_i(pool, PGF_EXPR_APP,
						PgfExprApp,
						.fun = expr, .arg = earg);
		}
		return expr;
	}
	case PGF_PRODUCTION_COERCE: {
		PgfProductionCoerce* pcoerce = pi.data;
		return pgf_cat_to_expr(pcoerce->coerce, choice, pool);
	}
	default:
		gu_impossible();
	}
	return gu_variant_null;
}


static PgfExpr
pgf_cat_to_expr(PgfCCat* cat, GuChoice* choice, GuPool* pool)
{
	if (cat->fid != PGF_FID_SYNTHETIC) {
		// XXX: What should the PgfMetaId be?
		return gu_variant_new_i(pool, PGF_EXPR_META,
					PgfExprMeta, 
					.id = 0);
	}
	int n_prods = pgf_production_seq_size(cat->prods);
	int i = gu_choice_next(choice, n_prods);
	if (i == -1) {
		return gu_variant_null;
	}
	PgfProduction prod = pgf_production_seq_get(cat->prods, i);
	return pgf_production_to_expr(prod, choice, pool);
}


PgfExpr
pgf_parse_result_next(PgfParseResult* pr, GuPool* pool)
{
	if (pr->choice == NULL) {
		return gu_variant_null;
	}
	int n_results = pgf_ccat_seq_size(pr->completed);
 	GuChoiceMark mark = gu_choice_mark(pr->choice);
	int i = gu_choice_next(pr->choice, n_results);
	if (i == -1) {
		return gu_variant_null;
	}
	PgfCCat* cat = pgf_ccat_seq_get(pr->completed, i);
	PgfExpr ret = pgf_cat_to_expr(cat, pr->choice, pool);
	gu_choice_reset(pr->choice, mark);
	if (!gu_choice_advance(pr->choice)) {
		pr->choice = NULL;
	};
	return ret;
}

PgfParseResult*
pgf_parse_result(PgfParse* parse, GuPool* pool)
{
	return gu_new_s(pool, PgfParseResult,
			.completed = parse->completed,
			.choice = gu_choice_new(pool));
}



// TODO: s/CId/Cat, add the cid to Cat, make Cat the key to CncCat
PgfParse*
pgf_parser_parse(PgfParser* parser, PgfCId cat, int lin_idx, GuPool* pool)
{
	PgfParse* parse = pgf_parse_new(parser, pool);
	GuPool* tmp_pool = gu_pool_new();
	PgfParsing* parsing = pgf_parsing_new(parse, pool, tmp_pool);
	PgfCncCat* cnccat = gu_strmap_get(parser->concr->cnccats, cat);
	if (!cnccat) {
		// error ...
		gu_impossible();
	}
	gu_assert(lin_idx == -1 || lin_idx < cnccat->n_lins);
	int n_ccats = gu_list_length(cnccat->cats);
	for (int i = 0; i < n_ccats; i++) {
		PgfCCat* ccat = gu_list_index(cnccat->cats, i);
		if (ccat != NULL) {
			pgf_parsing_predict(parsing, NULL, ccat, lin_idx);
		}
	}
	gu_pool_free(tmp_pool);
	return parse;
}

PgfParser* 
pgf_parser_new(PgfConcr* concr, GuPool* pool)
{
	gu_require(concr != NULL);
	PgfParser* parser = gu_new(pool, PgfParser);
	parser->concr = concr;
	return parser;
}
