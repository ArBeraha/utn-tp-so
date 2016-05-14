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
int stack_size(t_stack*);
void* stack_pop(t_stack*);
void stack_destroy(t_stack*); //todo fix memory leak de los items
void* stack_get(t_stack*,int);

#endif /* STACK_H_ */
