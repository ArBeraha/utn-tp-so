/*
 * commonTypes.c
 *
 *  Created on: 15/5/2016
 *      Author: utnso
 */

#include "commonTypes.h"

t_stack* stack_create() {
	return list_create();
}
void stack_push(t_stack* stack, t_stack_item* item) {
	list_add((t_list*)stack, (void*)item);
}
int stack_size(t_stack* stack){
	return list_size((t_list*)stack);
}
t_stack_item* stack_pop(t_stack* stack) {
	return (t_stack_item*)list_remove((t_list*)stack, stack_size(stack)-1);
}

t_stack_item* stack_get(t_stack* stack, int i){
	return (t_stack_item*)list_get((t_list*)stack,i);
}


void stack_item_destroy(t_stack_item* item){
	list_destroy(item->argumentos);
	dictionary_destroy(item->identificadores);
	free(item);
}

void stack_destroy(t_stack* stack){
	//todo fix memory leak de los items
	list_iterate((t_list*)stack,stack_item_destroy);
	list_destroy((t_list*)stack);
}

void pcb_destroy(t_PCB* pcb){
	list_destroy(pcb->indice_codigo);
	stack_destroy(pcb->SP);
	dictionary_destroy(pcb->indice_etiquetas);
	free(pcb);
}
