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
	char* semid = malloc(largo);
	read(clientes[cliente].socket, semid, largo);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (sem->valor > 0)
		sem->valor--;
	else {
		queue_push(sem->cola,(void*)cliente);
		// Designar espera
	}
	free(semid);
	free(semLen);
}
void signalSemaforo(int cliente) {
	// TODO DESBLOQUEAR CPU SI >0
	char* semLen = malloc(sizeof(int));
	read(clientes[cliente].socket, semLen, sizeof(int));
	int largo = char4ToInt(semLen);
	char* semid = malloc(largo);
	read(clientes[cliente].socket, semid, largo);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (!queue_is_empty(sem->cola)){
	int bloqueado = (int)queue_pop(sem->cola);
		// Desasignar espera
	}
	else
	sem->valor++;
	free(semid);
	free(semLen);
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
	int consola = ((t_proceso*) clientes[cliente].pid)->consola;
	char* serialValor = malloc( sizeof(ansisop_var_t));
	read(cliente, serialValor, sizeof(ansisop_var_t));
	char* name = malloc(sizeof(char));
	read(cliente, name, sizeof(char));
	char* serialHeader = headerToMSG(HeaderImprimirVariableConsola);
	send_w(consola, serialHeader, 1);
	send_w(consola, serialValor, sizeof(ansisop_var_t));
	send_w(consola, name, sizeof(char));
	free(serialHeader);
	free(serialValor);
	free(name);
}
void imprimirTexto(int cliente) {
	int consola = ((t_proceso*) clientes[cliente].pid)->consola;
	char* serialSize = malloc(sizeof(int));
	read(cliente, serialSize, sizeof(int));
	int size = char4ToInt(serialSize);
	char* texto = malloc(size);
	read(cliente, texto, size);
	char* serialHeader = headerToMSG(HeaderImprimirTextoConsola);
	send_w(consola, serialHeader, 1);
	send_w(consola, serialSize, sizeof(int));
	send_w(consola, texto, size);
	free(serialHeader);
	free(serialSize);
	free(texto);
}
void entradaSalida(int cliente) {
	char* serialSize = malloc(sizeof(int));
	read(clientes[cliente].socket, serialSize, sizeof(int));
	int size = char4ToInt(serialSize);
	char* serialIO = malloc(size);
	read(clientes[cliente].socket, serialIO, size);
	bloquearProceso(clientes[cliente].pid,serialIO);
	free(serialSize);
	free(serialIO);
}
