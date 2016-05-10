/*
 * commonTypes.h
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#ifndef OTROS_COMMONTYPES_H_
#define OTROS_COMMONTYPES_H_

#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
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
	int posicion;
	t_list* argumentos; // lista de t_variable
	t_dictionary* variables; // diccionario ['ID',variable]
	int posicionRetorno;
	t_variable valorRetorno;
} t_stack_item;

typedef struct {
	int PID; // identificador único
	int PC;	 // Program Counter
	t_stack* SP; // Posición del Stack: Pila de t_stack_item
	int cantidad_paginas;
	t_list* indice_codigo; // lista de t_sentencias que indican la posicion absoluta de una sentencia
	t_dictionary* indice_etiquetas; // diccionario [Etiqueta,Posicion de la sentencia a saltar]
} t_PCB;

/* ---------- INICIO DE SERIALIZACION ---------- */
void imprimir_serializacion(char* serial, int largo){
	int i;
	for (i=0;i<largo;i++){
		printf("[%d]",serial[i]);
	}
	printf("\n");
}
void serializar_int(char* destino,int* fuente){
	memcpy(destino,fuente,sizeof(int));
}
void deserializar_int(int* destino, char* fuente){
	memcpy(destino,fuente,sizeof(int));
}
void serializar_variable(char* destino,t_variable* fuente){
	memcpy(destino,fuente,sizeof(t_variable));
}
void deserializar_variable(t_variable* destino, char* fuente){
	memcpy(destino,fuente,sizeof(t_variable));
}
void serializar_list(char* destino,t_list* fuente, int pesoElemento){
	int i,offset=1;
	destino[0]=list_size(fuente); // Se va a guardar en la primera posicion la cantidad de elementos
	for (i=0;i<destino[0];i++){
		memcpy(destino+offset,list_get(fuente,i),pesoElemento);
		offset+=pesoElemento;
	}
}

void deserializar_list(t_list* destino,char* fuente, int pesoElemento){
	int i,offset=1;
	int size=fuente[0]; // Se va a guardar en la primera posicion la cantidad de elementos
	for (i=0;i<size;i++){
		char* buffer = malloc(pesoElemento);
		memcpy(buffer,fuente+offset,pesoElemento);
		list_add(destino,buffer);
		offset+=pesoElemento;
	}
}


/* ---------- FIN DE SERIALIZACION ---------- */

#endif /* OTROS_COMMONTYPES_H_ */
