/*
 * stack.c
 *
 *  Created on: 5/5/2016
 *      Author: utnso
 */

#include "stack.h"

t_stack* stack_create() {
	return list_create();
}

void stack_push(t_stack* stack, void* item) {
	list_add(stack, item);
}

void* stack_pop(t_stack* stack) {
	return list_remove(stack, list_size(stack)-1);
}
