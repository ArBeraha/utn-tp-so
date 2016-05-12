/*
 * serializacion.c
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#include "serializacion.h"

void imprimir_serializacion(char* serial, int largo) {
	int i;
	for (i = 0; i < largo; i++) {
		printf("[%d]", serial[i]);
	}
	printf("\n");
}
int serializar_int(char* destino, int* fuente) {
	memcpy(destino, fuente, sizeof(int));
	return sizeof(int);
}
int deserializar_int(int* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(int));
	return sizeof(int);
}
int serializar_variable(char* destino, t_variable* fuente) {
	memcpy(destino, fuente, sizeof(t_variable));
	return sizeof(t_variable);
}
int deserializar_variable(t_variable* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(t_variable));
	return sizeof(t_variable);
}
int serializar_sentencia(char* destino, t_sentencia* fuente) {
	memcpy(destino, fuente, sizeof(t_sentencia));
	return sizeof(t_sentencia);
}
int deserializar_sentencia(t_sentencia* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(t_sentencia));
	return sizeof(t_sentencia);

}
int serializar_list(char* destino, t_list* fuente, int pesoElemento) {
	int i, offset = 1;
	destino[0] = list_size(fuente); // Se va a guardar en la primera posicion la cantidad de elementos
	for (i = 0; i < destino[0]; i++) {
		memcpy(destino + offset, list_get(fuente, i), pesoElemento);
		offset += pesoElemento;
	}
	return offset;
}

int deserializar_list(t_list* destino, char* fuente, int pesoElemento) {
	int i, offset = 1;
	int size = fuente[0]; // Se va a guardar en la primera posicion la cantidad de elementos
	for (i = 0; i < size; i++) {
		char* buffer = malloc(pesoElemento);
		memcpy(buffer, fuente + offset, pesoElemento);
		list_add(destino, buffer);
		offset += pesoElemento;
	}
	return offset;
}

int bytes_stack_item(t_stack_item* fuente) {
	return sizeof(int) + list_size(fuente->argumentos) * sizeof(t_variable) + 1
			+ list_size(fuente->identificadores) * sizeof(t_identificador) + 1
			+ sizeof(int) + sizeof(t_variable);
}

int bytes_stack(t_stack* fuente) {
	int i, bytes = 0;
	for (i = 0; i < stack_size(fuente); i++) {
		bytes += bytes_stack_item(list_get(fuente, i));
	}
	return bytes;
}

int serializar_stack_item(char* destino, t_stack_item* fuente) {
	int offset = 0;
	offset += serializar_int(destino, fuente->posicion);
	offset += serializar_list(destino + offset, fuente->argumentos,	sizeof(t_variable));
	offset += serializar_list(destino + offset, fuente->identificadores, sizeof(t_identificador));
	offset += serializar_int(destino, fuente->posicionRetorno);
	offset += serializar_variable(destino, &(fuente->valorRetorno));
	return offset; // Retorna el offset
}

int serializar_stack(char* destino, t_stack* fuente) {
	int i, offset = 1;
	t_stack_item* item;
	destino[0] = stack_size(fuente); // Cantidad de items
	for (i = 0; i < destino[0]; i++) {
		item = list_get(fuente, i);
		offset += serializar_stack_item(destino + offset, item); // Serial del item
	}
	return offset;
}

int deserializar_stack_item(t_stack_item* destino, char* fuente) {
	int offset = 0;
	destino->argumentos = list_create();
	destino->identificadores = list_create();
	offset += deserializar_int(destino->posicion, fuente + offset);
	offset += deserializar_list(destino->argumentos, fuente + offset, sizeof(t_variable));
	offset += deserializar_list(destino->identificadores, fuente + offset, sizeof(t_identificador));
	offset += deserializar_int(destino->posicionRetorno, fuente + offset);
	offset += deserializar_variable(&(destino->valorRetorno), fuente + offset);
	return offset;
}

int deserializar_stack(t_stack* destino, char* fuente) {
	t_stack_item* item;
	int i, offset = 1; // Cantidad de items en fuente[0]
	for (i = 0; i < fuente[0]; i++) {
		item = malloc(sizeof(t_stack_item)); // Tamaño del item (o sea de sus punteros tambien [NO DE LAS LISTAS EN SI])
		offset += deserializar_stack_item(item, fuente + offset);
		stack_push(destino, item); // Agregar el item recibido al stack
	}
	return offset;
}

