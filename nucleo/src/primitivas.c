/*
 * primitivas.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include "nucleo.h"

void waitSemaforo(int cliente) {
	// TODO BLOQUEAR CPU SI <=0
	char* semid = leerLargoYMensaje(cliente);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (sem->valor > 0)
		sem->valor--;
	else
		bloquearProcesoSem(clientes[cliente].pid, semid);
	free(semid);
}
void signalSemaforo(int cliente) {
	// TODO DESBLOQUEAR CPU SI >0
	char* semid = leerLargoYMensaje(cliente);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (!queue_is_empty(sem->cola))
		desbloquearProceso((int)queue_pop(sem->cola));
	else
	sem->valor++;
	free(semid);
}
void asignarCompartida(int cliente) {
	char* compartida = leerLargoYMensaje(cliente);
	char* valor = malloc(sizeof(int));
	read(clientes[cliente].socket, valor, sizeof(int));
	*(int*) dictionary_get(tablaGlobales, compartida) = char4ToInt(valor);
	free(compartida);
	free(valor);
}
void devolverCompartida(int cliente) {
	char* compartida = leerLargoYMensaje(cliente);
	//send_w(clientes[cliente].socket, intToChar(HeaderDevolverCompartida),1); TODO crear el header
	char* valor = intToChar4(*(int*) dictionary_get(tablaGlobales, compartida));
	send_w(clientes[cliente].socket, valor, sizeof(int));
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
	char* texto = leerLargoYMensaje(cliente);
	char* serialHeader = headerToMSG(HeaderImprimirTextoConsola);
	send_w(consola, serialHeader, 1);
	enviarLargoYMensaje(cliente, texto);
	free(serialHeader);
	free(texto);
}
void entradaSalida(int cliente) {
	char* serialIO = leerLargoYMensaje(cliente);
	bloquearProcesoIO(clientes[cliente].pid,serialIO);
	free(serialIO);
}
