#include "intern.h"

struct GuIntern {
	GuPool* str_pool;
	GuStringMap* map;
};

GuIntern*
gu_intern_new(GuPool* pool, GuPool* str_pool)
{
	GuIntern* intern = gu_new(pool, GuIntern);
	intern->str_pool = str_pool;
	intern->map = gu_stringmap_new(pool);
	return intern;
}

GuString*
gu_intern_string(GuIntern* intern, GuCString* cstr)
{
	GuString* str = gu_stringmap_get(intern->map, cstr);
	if (str == NULL) {
		str = gu_string_copy(intern->str_pool, cstr);
		gu_stringmap_set(intern->map, str, str);
	}
	return str;
}
