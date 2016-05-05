/*
 * stack.h
 *
 *  Created on: 5/5/2016
 *      Author: utnso
 */

#ifndef STACK_H_
#define STACK_H_

#include <commons/collections/list.h>

typedef t_list t_stack;
t_stack* stack_create();
void stack_push(t_stack*,void*);
void* stack_pop(t_stack*);

#endif /* STACK_H_ */
