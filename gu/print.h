// Copyright 2012 University of Helsinki. Released under LGPL3.

#ifndef GU_PRINT_H_
#define GU_PRINT_H_

#include <gu/write.h>

typedef const struct GuPrinter GuPrinter;

struct GuPrinter {
	void (*print)(GuPrinter* self, const void* p, GuWriter* wtr, GuExn* exn);
};

typedef const struct GuPrintSpec GuPrintSpec;

struct GuPrintSpec {
	const char* text;
	GuPrinter* printer;
};

typedef const struct GuFmt GuFmt;
struct GuFmt {
	size_t len;
	GuPrintSpec* data;
};

void
gu_print_fmt(GuFmt* fmt, const void** args, GuWriter* wtr, GuExn* exn);

#define gu_with_fmt_(FMT, FARGS, FVAR, AVAR, BODY)	\
	GU_BEGIN					\
	static const GuPrintSpec FVAR##_data_[] = { FMT };	\
	static const GuFmt FVAR = {				\
		.len = sizeof(FVAR##_data_) / sizeof(GuPrintSpec),	\
		.data = FVAR##_data_					\
	}; \
	const void* AVAR[sizeof((GuPrintSpec[]){FMT})/sizeof(GuPrintSpec)] = \
		FARGS;							\
	BODY; \
	GU_END
			   
#define gu_print(WTR, EXN, FMT, ...)		\
	gu_with_fmt(FMT, GU_B(__VA_ARGS__), gu_fmt_, gu_fargs_,		\
		    gu_print_fmt(&gu_fmt_, gu_fargs_, WTR, EXN))


extern GuPrinter gu_size_printer[1];
extern GuPrinter gu_string_printer[1];

#endif /* GU_PRINT_H_ */
