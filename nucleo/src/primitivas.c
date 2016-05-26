/*
 * primitivas.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */

#include "nucleo.h"

void waitSemaforo(int cliente) {
	// TODO BLOQUEAR CPU SI <=0
	char* semLen = malloc(sizeof(int));
	int largo = char4ToInt(semLen);
	read(clientes[cliente].socket, semLen, sizeof(int));
	char* sem = malloc(largo);
	read(clientes[cliente].socket, sem, largo);
	int* valor = (int*) dictionary_get(tablaSEM, sem);
	if ((*valor) > 0) {
		//CongelarCPU()
		(*valor)--;
	}
}
void signalSemaforo(int cliente) {
	// TODO DESBLOQUEAR CPU SI >0
	char* semLen = malloc(sizeof(int));
	int largo = char4ToInt(semLen);
	read(clientes[cliente].socket, semLen, sizeof(int));
	char* sem = malloc(largo);
	read(clientes[cliente].socket, sem, largo);
	//if (hayCPUsCongeladas)
	//descongelarUnaCPU()
	//else
	(*(int*) dictionary_get(tablaSEM, sem))++;
}
void asignarCompartida(int cliente) {
	char* varLen = malloc(sizeof(int));
	int largo = char4ToInt(varLen);
	read(clientes[cliente].socket, varLen, sizeof(int));
	char* compartida = malloc(largo);
	read(clientes[cliente].socket, compartida, largo);
	char* valor = malloc(sizeof(int));
	read(clientes[cliente].socket, valor, sizeof(int));
	*(int*) dictionary_get(tablaGlobales, compartida) = char4ToInt(valor);
	free(varLen);
	free(compartida);
	free(valor);
}
void devolverCompartida(int cliente) {
	char* varLen = malloc(sizeof(int));
	int largo = char4ToInt(varLen);
	read(clientes[cliente].socket, varLen, sizeof(int));
	char* compartida = malloc(largo);
	read(clientes[cliente].socket, compartida, largo);
	//send_w(clientes[cliente].socket, intToChar(HeaderDevolverCompartida),1); TODO crear el header
	char* valor = intToChar4(*(int*) dictionary_get(tablaGlobales, compartida));
	send_w(clientes[cliente].socket, valor, sizeof(int));
	free(varLen);
	free(compartida);
	free(valor);
}
