#include "intern.h"

struct GuIntern {
	GuPool* str_pool;
	GuStrMap* map;
};

GuIntern*
gu_intern_new(GuPool* pool, GuPool* str_pool)
{
	GuIntern* intern = gu_new(pool, GuIntern);
	intern->str_pool = str_pool;
	intern->map = gu_strmap_new(pool);
	return intern;
}

const char*
gu_intern_str(GuIntern* intern, const char* cstr)
{
	char* str = gu_strmap_get(intern->map, (char*) cstr);
	if (str == NULL) {
		str = gu_strdup(cstr, intern->str_pool);
		gu_strmap_set(intern->map, str, str);
	}
	return str;
}
