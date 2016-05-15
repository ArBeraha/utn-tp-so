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
#include "stack.h" // Ya que no tenemos pila, hice un nuevo tipo que funciona de esa manera: t_stack

typedef int ansisop_var_t;

// ^ @LucasEsposito creo que es lo que empezaste a hacer con ansisop_var_t?
typedef struct { // El ID queda a cargo del stack, no en todos lados las variables tienen ID, por ejemplo cuando son argumentos de una funcion
	int pagina;
	int offset;
	int size;
} __attribute__((packed)) t_variable;

typedef struct {
	int offset_inicio;
	int offset_fin;
} __attribute__((packed)) t_sentencia;

typedef struct {
	char ID;
	t_variable variable;
} __attribute__((packed)) t_identificador;

typedef struct {
	int posicion;
	t_list* argumentos; // lista de t_variable
	t_list* identificadores;  // lista de t_identificador
	int posicionRetorno;
	t_variable valorRetorno;
} t_stack_item;



typedef struct {
	int PID; // identificador único
	int PC;	 // Program Counter
	t_stack* SP; // Posición del Stack: Pila de t_stack_item
	int cantidad_paginas;
	t_list* indice_codigo; // lista de t_sentencias que indican la posicion absoluta de cada sentencia
	t_dictionary* indice_etiquetas; // diccionario [Etiqueta,Posicion de la sentencia a saltar]
} t_PCB;


void pcb_destroy(t_PCB* pcb);

#endif /* OTROS_COMMONTYPES_H_ */
