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
void serializar_int(char* destino, int* fuente) {
	memcpy(destino, fuente, sizeof(int));
}
void deserializar_int(int* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(int));
}
void serializar_variable(char* destino, t_variable* fuente) {
	memcpy(destino, fuente, sizeof(t_variable));
}
void deserializar_variable(t_variable* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(t_variable));
}
void serializar_sentencia(char* destino, t_sentencia* fuente) {
	memcpy(destino, fuente, sizeof(t_sentencia));
}
void deserializar_sentencia(t_sentencia* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(t_sentencia));
}
void serializar_list(char* destino, t_list* fuente, int pesoElemento) {
	int i, offset = 1;
	destino[0] = list_size(fuente); // Se va a guardar en la primera posicion la cantidad de elementos
	for (i = 0; i < destino[0]; i++) {
		memcpy(destino + offset, list_get(fuente, i), pesoElemento);
		offset += pesoElemento;
	}
}

void deserializar_list(t_list* destino, char* fuente, int pesoElemento) {
	int i, offset = 1;
	int size = fuente[0]; // Se va a guardar en la primera posicion la cantidad de elementos
	for (i = 0; i < size; i++) {
		char* buffer = malloc(pesoElemento);
		memcpy(buffer, fuente + offset, pesoElemento);
		list_add(destino, buffer);
		offset += pesoElemento;
	}
}

int bytes_stack_item(t_stack_item* fuente) {
	return sizeof(int) + list_size(fuente->argumentos) * sizeof(t_variable)
			+ list_size(fuente->identificadores) * sizeof(t_identificador)
			+ sizeof(int) + sizeof(t_variable);
}

int bytes_stack(t_stack* fuente) {
	int i, bytes = 0;
	t_stack_item* item;
	for (i = 0; i < stack_size(fuente); i++) {
		item = list_get(fuente, i);
		bytes += bytes_stack_item(item);
	}
	return bytes;
}

void serializar_stack_item(char* destino, t_stack_item* fuente) {
	int offset = 0;
	serializar_int(destino, fuente->posicion);
	offset += sizeof(int);
	destino[offset++] = list_size(fuente->argumentos);
	int bytes = list_size(fuente->argumentos) * sizeof(t_variable);
	serializar_list(destino + offset, fuente->argumentos, sizeof(t_variable));
	offset += bytes;
	destino[offset++] = list_size(fuente->identificadores);
	bytes = list_size(fuente->identificadores) * sizeof(t_identificador);
	serializar_list(destino + offset, fuente->argumentos,
			sizeof(t_identificador));
	offset += bytes;
	serializar_int(destino, fuente->posicionRetorno);
	offset += sizeof(int);
	serializar_variable(destino, &(fuente->valorRetorno));
	offset += sizeof(t_variable);
}

void serializar_stack(char* destino, t_stack* fuente) {
	int i, offset = 1;
	t_stack_item* item;
	destino[0] = stack_size(fuente); // Se va a guardar en la primera posicion la cantidad de elementos
	for (i = 0; i < destino[0]; i++) {
		item = list_get(fuente, i);
		serializar_stack_item(destino + offset, item);
		offset += bytes_stack_item(item);
	}
}

