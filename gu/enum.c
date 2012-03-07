#include <gu/enum.h>

bool
gu_enum_next(GuEnum* en, void* to, GuPool* pool)
{
	return en->next(en, to, pool);
}
