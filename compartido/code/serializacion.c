/*
 * serializacion.c
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#include "serializacion.h"

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
