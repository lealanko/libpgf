#include "choice.h"

#include <glib.h>

struct GuChoice {
	GByteArray* path;
	int path_idx;
};

static void 
gu_choice_byte_array_free_cb(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	g_byte_array_unref(clo->env1);
}

GuChoice*
gu_choice_new(GuPool* pool)
{
	GuChoice* ch = gu_new(pool, GuChoice);
	ch->path_idx = 0;
	ch->path = g_byte_array_new();
	GuClo1* clo = gu_new_s(pool, GuClo1,
			       gu_choice_byte_array_free_cb, ch->path);
	gu_pool_finally(pool, &clo->fn);
	return ch;
}

GuChoiceMark
gu_choice_mark(GuChoice* ch)
{
	gu_assert(ch->path_idx <= ch->path->len);
	return (GuChoiceMark){ch->path_idx};
}

void
gu_choice_reset(GuChoice* ch, GuChoiceMark mark)
{
	gu_assert(ch->path_idx <= ch->path->len);
	gu_assert(mark->path_idx <= ch->path_idx);
	gu_assert(mark->path_idx >= 0);
	ch->path_idx = mark->path_idx;
}

int
gu_choice_next(GuChoice* ch, int n_choices)
{
	gu_assert(n_choices >= 0);
	gu_assert(ch->path_idx <= ch->path->len);
	if (n_choices == 0) {
		return -1;
	}
	int i = 0;
	if (ch->path->len > ch->path_idx) {
		i = (int) ch->path->data[ch->path_idx];
		gu_assert(i <= n_choices);
	} else {
		g_byte_array_append(ch->path, &(guint8){n_choices}, 1);
		i = n_choices;
	}
	return (i == 0) ? -1 : n_choices - i;
}

bool
gu_choice_advance(GuChoice* ch)
{
	gu_assert(ch->path_idx <= ch->path->len);
	
	while (ch->path->len > ch->path_idx) {
		int end = (int) ch->path->len - 1;
		if (ch->path->data[end] <= 1) {
			g_byte_array_set_size(ch->path, end);
		} else {
			ch->path->data[end]--;
			return true;
		}
	}
	return false;
}
