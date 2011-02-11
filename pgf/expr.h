#ifndef PGF_EXPR_H_
#define PGF_EXPR_H_

#include <pgf/data.h>
#include <gu/variant.h>

#define APP(f, a) \
	gu_variant_new_s(PGF_EXPR_POOL, PGF_EXPR_APP, PgfExprApp, f, a)
#define APP2(f, a1, a2) APP(APP(f, a1), a2)
#define APP3(f, a1, a2, a3) APP2(APP(f, a1), a2, a3)

#define VAR(s) \
	gu_variant_new_i(PGF_EXPR_POOL, PGF_EXPR_FUN, PgfExprFun, gu_cstring(#s))

#define APPV(s, a) APP(VAR(s), a)
#define APPV2(s, a1, a2) APP2(VAR(s), a1, a2)
#define APPV3(s, a1, a2, a3) APP3(VAR(s), a1, a2)



#endif // PGF_EXPR_H_
