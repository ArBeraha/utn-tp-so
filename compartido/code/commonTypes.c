/*
 * commonTypes.c
 *
 *  Created on: 15/5/2016
 *      Author: utnso
 */

#include "commonTypes.h"

t_stack* stack_create() {
	t_list* stack = list_create();
//	t_stack_item* head = stack_item_create();
//	head->posicion=0;
//	stack_push(stack,head);
	return stack;
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

void color_print(char* texto){
	system(string_from_format("echo \"\e[1m\e[5m\e[31m%s\e[0m\"", texto));
}

void imprimir_PCB(t_PCB* pcb){
	printf("[Inicio PCB]\n");
	printf("\tPC:%d\n",pcb->PC);
	printf("\tPID:%d\n",pcb->PID);
	printf("\tCantidad_Paginas:%d\n",pcb->cantidad_paginas);
	printf("\t[Inicio Stack]\n");
	int i,e;
	for (i = 0; i < stack_size(pcb->SP); i++) {
		printf("\t\t[Inicio Stack Item]\n");
		t_stack_item* item = stack_get(pcb->SP, i);
		printf("\t\t\t[Inicio Argumentos]\n");

		for (e = 0; e < list_size(item->argumentos); e++) {
			t_pedido* pedido = (t_pedido*) list_get(item->argumentos, e);
			printf("\t\t\t\tARGUMENTO:(%d,%d,%d)\n", pedido->pagina, pedido->offset,
					pedido->size);
		}
		printf("\t\t\t[Fin Argumentos]\n");
		printf("\t\t\t[Inicio Identificadores]\n");

		int table_index;
		for (table_index = 0; table_index < item->identificadores->table_max_size; table_index++) {
			t_hash_element *element = item->identificadores->elements[table_index];

			while (element != NULL) {
				t_pedido* pedido =	(t_pedido*) element->data;
				printf("\t\t\t\tIDENTIFICADOR:%s PEDIDO:(%d,%d,%d)\n",element->key,pedido->pagina,pedido->offset, pedido->size);
				//Siguiente elemento
				element = element->next;
			}
		}
	printf("\t\t\t[Fin Identificadores]\n");
	printf("\t\t\tPosicion:%d\n",item->posicion);
	printf("\t\t\tPosicionRetorno:%d\n",item->posicionRetorno);
	printf("\t\t\tvalorRetorno:(%d,%d,%d)\n",item->valorRetorno.pagina, item->valorRetorno.offset,item->valorRetorno.size);
	printf("\t\t[Fin Stack Item]\n");
	};
	printf("\t[Fin Stack]\n");

	printf("\t[Inicio Indice Codigo]\n");
	for (e = 0; e < list_size(pcb->indice_codigo); e++) {
		t_sentencia* sentencia = (t_sentencia*) list_get(pcb->indice_codigo, e);
		printf("\t\t\tSENTENCIA:(%d,%d)\n",sentencia->offset_inicio,sentencia->offset_fin);
	}
	printf("\t[Fin Indice Codigo]\n");
	printf("\t[Inicio Indice Etiquetas]\n");
	int table_index;
	for (table_index = 0; table_index < pcb->indice_etiquetas->table_max_size; table_index++) {
		t_hash_element *element = pcb->indice_etiquetas->elements[table_index];

		while (element != NULL) {
			printf("\t\t\tETIQUETA:%s SALTO:%d\n",element->key,*(int*)element->data);
			//Siguiente elemento
			element = element->next;
		}
	}
	printf("\t[Fin Indice Etiquetas]\n");
	printf("[Fin PCB]\n");

}
