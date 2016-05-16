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
	list_add((t_list*)stack, item);
}
int stack_size(t_stack* stack){
	return list_size((t_list*)stack);
}
void* stack_pop(t_stack* stack) {
	return list_remove((t_list*)stack, stack_size(stack)-1);
}

void* stack_get(t_stack* stack, int i){
	return list_get((t_list*)stack,i);
}

// El destroy esta en commonTypes.c
