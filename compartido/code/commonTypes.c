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
				(list_size(item->argumentos) + dictionary_size(item->identificadores))
						* sizeof(int);
	}
	return bytes;
}

t_pedido* stack_next_pedido(t_stack* fuente, int pagsize) {
	// Rompe todo si las paginas no son multiplos de 4
	t_pedido* pedido = malloc(sizeof(t_pedido));
	int actualSize = stack_memory_size(fuente);
	pedido->pagina = actualSize / pagsize;
	pedido->offset = actualSize - pedido->pagina * pagsize;
	pedido->size = sizeof(int);
	return pedido;
}

t_pedido* stack_max_pedido(t_stack* stack, int pagsize){ //Retorna el ultimo pedido hecho
	t_pedido* pedido = malloc(sizeof(t_pedido));
	int posicionUltimo;
	t_stack_item* head = stack_head(stack);

	t_pedido* nextPedido = stack_next_pedido(stack, pagsize);
	if(pedido->offset>=sizeof(int)){ // "Vuelvo <<un lugar>> para atras respecto del pedido que seguiria"
		pedido->offset -= sizeof(int);
	}else{ //fixme: rompe si las pags no son multiplos de 4. Lo agrego al final de la ultima pagina, porque el proximo pedido va al inicio de la primera.
		pedido->pagina -= 1;
		pedido->offset = pagsize - sizeof(int);
	}
	return pedido;
}


t_stack_item* stack_item_create(){
	t_stack_item* item = malloc(sizeof(t_stack_item));
	item->argumentos = list_create();
	item->identificadores = dictionary_create();
	return item;
}


void stack_destroy(t_stack* stack){
	//todo fix memory leak de los items
	list_iterate((t_list*)stack,(void*)stack_item_destroy);
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

void list_remove_by_value(t_list* list, void* value) {
	int i;
	for (i = 0; i < list_size(list); i++) {
		if (list_get(list, i) == value) {
			list_remove(list, i);
			break;
		}
	}
}
