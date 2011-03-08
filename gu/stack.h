#ifndef GU_STACK_H_
#define GU_STACK_H_

#include <gu/mem.h>

typedef struct GuStack GuStack;

GuStack* gu_stack_alloc(GuPool* pool, size_t elem_size);

#define gu_stack_new(pool, t_) \
	gu_stack_alloc(pool, sizeof(t_))

void*
gu_stack_push(GuStack* stack);

void*
gu_stack_peek(GuStack* stack);

void 
gu_stack_pop(GuStack* stack);

bool
gu_stack_is_empty(GuStack* stack);

#endif // GU_STACK_H_
