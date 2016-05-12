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
	return sizeof(int) + list_size(fuente->argumentos) * sizeof(t_variable) +1
			+ list_size(fuente->identificadores) * sizeof(t_identificador) +1
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

int serializar_stack_item(char* destino, t_stack_item* fuente) {
	int offset = 0;
	serializar_int(destino, fuente->posicion);
	offset += sizeof(int);
	destino[offset++] = list_size(fuente->argumentos);
	int bytes = list_size(fuente->argumentos) * sizeof(t_variable);
	serializar_list(destino + offset, fuente->argumentos, sizeof(t_variable));
	offset += bytes;
	destino[offset++] = list_size(fuente->identificadores);
	bytes = list_size(fuente->identificadores) * sizeof(t_identificador);
	serializar_list(destino + offset, fuente->identificadores,
			sizeof(t_identificador));
	offset += bytes;
	serializar_int(destino, fuente->posicionRetorno);
	offset += sizeof(int);
	serializar_variable(destino, &(fuente->valorRetorno));
	offset += sizeof(t_variable);
	return offset; // Retorna el offset
}

void serializar_stack(char* destino, t_stack* fuente) {
	int i, offset = 1;
	t_stack_item* item;
	destino[0] = stack_size(fuente); // Cantidad de items
	for (i = 0; i < destino[0]; i++) {
		item = list_get(fuente, i);
		offset+=serializar_stack_item(destino + offset, item); // Serial del item
	}
}

int deserializar_stack_item(t_stack_item* destino, char* fuente){
	int offset=0;
	deserializar_int(destino->posicion,fuente[offset]);
	offset+=sizeof(int);
	destino->argumentos = list_create();
	deserializar_list(destino->argumentos,fuente[offset],sizeof(t_variable));
	offset+=sizeof(t_variable)*list_size(destino->argumentos)+1; // +1 por la cantidad de elementos de la lista
	destino->identificadores = list_create();
	deserializar_list(destino->identificadores,fuente[offset],sizeof(t_identificador));
	offset+=sizeof(t_identificador)*list_size(destino->identificadores)+1; // +1 por la cantidad de elementos de la lista
	deserializar_int(destino->posicionRetorno,fuente[offset]);
	offset+=sizeof(int);
	deserializar_variable(destino->valorRetorno,fuente[offset]);
	offset+=sizeof(t_variable);
	return offset;
}

void deserializar_stack(t_stack* destino, char* fuente){
	t_stack_item* item;
	int i,offset=1; // Cantidad de items en fuente[0]
	for (i = 0; i < fuente[0]; i++) {
		item = malloc(sizeof(t_stack_item)); // Tamaño del item (o sea de sus punteros tambien [NO DE LAS LISTAS EN SI])
		offset+=deserializar_stack_item(item,fuente+offset);
		stack_push(destino,item); // Agregar el item recibido al stack
	}
}



