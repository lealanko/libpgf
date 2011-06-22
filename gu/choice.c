#include <gu/choice.h>
#include <gu/seq.h>
#include <gu/assert.h>

struct GuChoice {
	GuByteSeq* path;
	int path_idx;
};

GuChoice*
gu_choice_new(GuPool* pool)
{
	GuChoice* ch = gu_new(pool, GuChoice);
	ch->path = gu_byte_seq_new(pool);
	ch->path_idx = 0;
	return ch;
}

GuChoiceMark
gu_choice_mark(GuChoice* ch)
{
	gu_assert(ch->path_idx <= gu_byte_seq_size(ch->path));
	return (GuChoiceMark){ch->path_idx};
}

bool
gu_choice_reset(GuChoice* ch, GuChoiceMark mark)
{
	gu_assert(ch->path_idx <= gu_byte_seq_size(ch->path));
	gu_assert(mark.path_idx >= 0);
	if (mark.path_idx <= ch->path_idx) {
		ch->path_idx = mark.path_idx;
		return true;
	} else {
		return false;
	}
}

int
gu_choice_next(GuChoice* ch, int n_choices)
{
	gu_assert(n_choices >= 0);
	gu_assert(ch->path_idx <= gu_byte_seq_size(ch->path));
	if (n_choices == 0) {
		return -1;
	}
	int i = 0;
	if (gu_byte_seq_size(ch->path) > ch->path_idx) {
		i = (int) *gu_byte_seq_index(ch->path, ch->path_idx);
		gu_assert(i <= n_choices);
	} else {
		gu_byte_seq_push(ch->path, n_choices);
		i = n_choices;
	}
	ch->path_idx++;
	return (i == 0) ? -1 : n_choices - i;
}

bool
gu_choice_advance(GuChoice* ch)
{
	gu_assert(ch->path_idx <= gu_byte_seq_size(ch->path));
	
	while (gu_byte_seq_size(ch->path) > ch->path_idx) {
		uint8_t last = gu_byte_seq_pop(ch->path);
		if (last > 1) {
			gu_byte_seq_push(ch->path, last-1);
			return true;
		}
	}
	return false;
}
