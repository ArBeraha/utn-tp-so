/*
 * commonTypes.c
 *
 *  Created on: 15/5/2016
 *      Author: utnso
 */

#include "commonTypes.h"

void pcb_destroy(t_PCB* pcb){
	list_destroy(pcb->indice_codigo);
	stack_destroy(pcb->SP);
	dictionary_destroy(pcb->indice_etiquetas);
	free(pcb);
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
