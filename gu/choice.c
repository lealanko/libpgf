#include "choice.h"

struct GuChoice {
	GByteArray* path;
	int path_idx;
};

GuChoice*
gu_choice_new(GuPool* pool)
{
	GuChoice* ch = gu_new(pool, GuChoice);
	ch->path = g_byte_array_new();
	ch->path_idx = 0;
	return ch;
}

GuChoiceMark
gu_choice_mark(GuChoice* ch)
{
	return (GuChoiceMark) { ch->path_idx };
}

void
gu_choice_reset(GuChoice* ch, GuChoiceMark mark)
{
	gu_assert(ch->path_idx >= mark->path_idx);
	ch->path_idx = mark->path_idx;
}

int
gu_choice_next(GuChoice* ch, int n_choices);

bool
gu_choice_advance(GuChoice* ch, GuChoiceMark mark)
{
	
}
