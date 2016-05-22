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

t_stack_item* stack_head(t_stack* stack){
	return stack_get(stack,stack_size(stack)-1);
}


void stack_item_destroy(t_stack_item* item){
	list_destroy(item->argumentos);
	dictionary_destroy(item->identificadores);
	free(item);
}

int stack_memory_size(t_stack* fuente) {
	int i, bytes = 0;
	for (i = 0; i < stack_size(fuente); i++) {
		t_stack_item* item = stack_get(fuente, i);
		bytes +=
				(list_size(item->argumentos) + list_size(item->identificadores))
						* sizeof(int);
	}
	return bytes;
}

t_pedido stack_next_pedido(t_stack* fuente, int pagsize) {
	t_pedido pedido;
	int actualSize = stack_memory_size(fuente);
	pedido.pagina = actualSize / pagsize;
	pedido.offset = actualSize - pedido.pagina * pagsize;
	pedido.size = sizeof(int);
	return pedido;
}


void stack_destroy(t_stack* stack){
	//todo fix memory leak de los items
	list_iterate((t_list*)stack,stack_item_destroy);
	list_destroy((t_list*)stack);
}

t_PCB* pcb_create() {
	// NO SE USA PARA DESERIALIZAR YA QUE DESERIALIZAR_PCB() YA CREA LAS ESTRUCTURAS DINAMICAS
	// POR ENDE ES UN METODO EXCLUSIVO DE NUCLEO (referenciar a un issue para cambiar esto)
	// fixme
	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->PC=0;
	pcb->PID=0;
	pcb->cantidad_paginas=0;
	pcb->SP = stack_create();
	pcb->indice_codigo = list_create();
	pcb->indice_etiquetas = dictionary_create();
	return pcb;
}

void pcb_destroy(t_PCB* pcb){
	list_destroy(pcb->indice_codigo);
	stack_destroy(pcb->SP);
	dictionary_destroy(pcb->indice_etiquetas);
	free(pcb);
}
