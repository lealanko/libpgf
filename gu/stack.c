#include "stack.h"
#include "fun.h"
#include <glib.h>
#include <string.h>

struct GuStack {
	GArray* arr;
	size_t elem_size;
};

static void
gu_stack_free_fn(GuFn* fnp)
{
	GuClo1* clo = (GuClo1*) fnp;
	g_array_free(clo->env1, TRUE);
}

GuStack* 
gu_stack_alloc(GuPool* pool, size_t elem_size)
{
	GuStack* stack = gu_new(pool, GuStack);
	stack->elem_size = elem_size;
	stack->arr = g_array_new(FALSE, FALSE, elem_size);
	GuClo1* clo = gu_new_s(pool, GuClo1, gu_stack_free_fn, stack->arr);
	gu_pool_finally(pool, &clo->fn);
	return stack;
}

bool
gu_stack_is_empty(GuStack* stack)
{
	return (stack->arr->len == 0);
}

void*
gu_stack_peek(GuStack* stack)
{
	g_assert(!gu_stack_is_empty(stack));
	return &stack->arr->data[stack->elem_size * (stack->arr->len - 1)];
}

void*
gu_stack_push(GuStack* stack)
{
	g_array_set_size(stack->arr, stack->arr->len + 1);
	return gu_stack_peek(stack);
}

void
gu_stack_pop(GuStack* stack)
{
	g_assert(!gu_stack_is_empty(stack));
	g_array_set_size(stack->arr, stack->arr->len - 1);
}

