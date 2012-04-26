#ifndef GU_ENUM_H_
#define GU_ENUM_H_

#include <gu/mem.h>

/** @file
 * Enumerations.
 */

typedef struct GuEnum GuEnum;
/**< An enumeration of elements. */

struct GuEnum {
	bool (*next)(GuEnum* self, void* to, GuPool* pool);
};


bool
gu_enum_next(GuEnum* en, void* to, GuPool* pool);
/**< Get the next element from an enumeration. If the enumeration `en` is at
 * the end, `false` is returned. Otherwise, the next element of the
 * enumeration is stored in `to`, and `true` is returned.
 *
 * If the element is created during the function call, it is allocated from
 * `pool`. It may also depend on other objects.
 */



#endif /* GU_ENUM_H_ */
