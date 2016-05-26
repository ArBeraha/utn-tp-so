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
	read(clientes[cliente].socket, semLen, sizeof(int));
	int largo = char4ToInt(semLen);
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
	read(clientes[cliente].socket, semLen, sizeof(int));
	int largo = char4ToInt(semLen);
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
void imprimirVariable(int cliente) {
	int consola = 666;//= getConsolaAsociada(cliente);
	char* msgValue = malloc( sizeof(ansisop_var_t));
	read(cliente, msgValue, sizeof(ansisop_var_t));
	char* name = malloc(sizeof(char));
	read(cliente, name, sizeof(char));
	char* serialHeader = headerToMSG(HeaderImprimirVariableConsola);
	send_w(consola, serialHeader, 1);
	send_w(consola, msgValue, sizeof(ansisop_var_t));
	send_w(consola, name, sizeof(char));
	free(serialHeader);
	free(msgValue);
	free(name);
}
void imprimirTexto(int cliente) {
	int consola = 666; //= getConsolaAsociada(cliente);
	char* msgSize = malloc(sizeof(int));
	read(cliente, msgSize, sizeof(int));
	int size = char4ToInt(msgSize);
	char* texto = malloc(size);
	read(cliente, texto, size);
	char* serialHeader = headerToMSG(HeaderImprimirTextoConsola);
	send_w(consola, serialHeader, 1);
	send_w(consola, msgSize, sizeof(int));
	send_w(consola, texto, size);
	free(serialHeader);
	free(msgSize);
	free(texto);
}
void entradaSalida(int cliente) {
	char* serialSize = malloc(sizeof(int));
	read(clientes[cliente].socket, serialSize, sizeof(int));
	int size = char4ToInt(serialSize);
	char* io = malloc(size);
	read(clientes[cliente].socket, io, size);
	bloquearProceso(666,io); //Fixme Necesito obtener el PID de una cpu
}
