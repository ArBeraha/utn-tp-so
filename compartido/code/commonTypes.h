/*
 * commonTypes.h
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#ifndef OTROS_COMMONTYPES_H_
#define OTROS_COMMONTYPES_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>


typedef int ansisop_var_t;

// ^ @LucasEsposito creo que es lo que empezaste a hacer con ansisop_var_t?
typedef struct { // El ID queda a cargo del stack, no en todos lados las variables tienen ID, por ejemplo cuando son argumentos de una funcion
	int pagina;
	int offset;
	int size;
} __attribute__((packed)) t_pedido;

typedef struct {
	int offset_inicio;
	int offset_fin;
} __attribute__((packed)) t_sentencia;

typedef t_list t_stack;

typedef struct {
	int posicion;
	t_list* argumentos; // lista de t_pedido
	t_dictionary* identificadores;  // dictionary de ["Identificador",t_pedido]
	int posicionRetorno;
	t_pedido valorRetorno;
} t_stack_item;

typedef struct {
	int PID; // identificador único
	int PC;	 // Program Counter
	t_stack* SP; // Posición del Stack: Pila de t_stack_item
	int cantidad_paginas;
	t_list* indice_codigo; // lista de t_sentencias que indican la posicion absoluta de cada sentencia
	t_dictionary* indice_etiquetas; // diccionario [Etiqueta,Posicion de la sentencia a saltar]
} t_PCB;

t_PCB* pcb_create();
void pcb_destroy(t_PCB* pcb);
t_stack* stack_create();
void stack_push(t_stack*,t_stack_item*);
int stack_size(t_stack*);
t_stack_item* stack_pop(t_stack*);
t_stack_item* stack_get(t_stack*,int);
t_stack_item* stack_head(t_stack* stack);
void stack_destroy(t_stack*);
void stack_item_destroy(t_stack_item* item);
int stack_memory_size(t_stack* fuente);
t_pedido* stack_next_pedido(t_stack* fuente, int pagsize);
t_pedido* stack_max_pedido(t_stack* stack, int pagsize);
t_stack_item* stack_item_create();
void list_remove_by_value(t_list* list,void* value);

#endif /* OTROS_COMMONTYPES_H_ */
