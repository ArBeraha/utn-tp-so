/*
 * primitivas.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include "nucleo.h"

void waitSemaforo(int cliente) {
	char* semid = leerLargoYMensaje(clientes[cliente].socket);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (sem->valor > 0)
		sem->valor--;
	else
		bloquearProcesoSem(clientes[cliente].pid, semid);
	free(semid);
}
void signalSemaforo(int cliente) {
	char* semid = leerLargoYMensaje(clientes[cliente].socket);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (!queue_is_empty(sem->cola))
		desbloquearProceso((int)queue_pop(sem->cola));
	else
	sem->valor++;
	free(semid);
}
void asignarCompartida(int cliente) {
	char* compartida = leerLargoYMensaje(clientes[cliente].socket);
	char* valor = malloc(sizeof(int));
	read(clientes[cliente].socket, valor, sizeof(int));
	*(int*) dictionary_get(tablaGlobales, compartida) = char4ToInt(valor);
	free(compartida);
	free(valor);
}
void devolverCompartida(int cliente) {
	char* compartida = leerLargoYMensaje(clientes[cliente].socket);
	char* valor = intToChar4(*(int*) dictionary_get(tablaGlobales, compartida));
	send_w(clientes[cliente].socket, valor, sizeof(int));
	free(compartida);
	free(valor);
}
void imprimirVariable(int cliente) {
	int socketConsola = clientes[((t_proceso*) clientes[cliente].pid)->consola].socket;
	char* serialValor = malloc( sizeof(ansisop_var_t));
	read(cliente, serialValor, sizeof(ansisop_var_t));
	enviarHeader(socketConsola, HeaderImprimirVariableConsola);
	send_w(socketConsola, serialValor, sizeof(ansisop_var_t));
	free(serialValor);
}
void imprimirTexto(int cliente) {
	int socketConsola = clientes[((t_proceso*) clientes[cliente].pid)->consola].socket;
	char* texto = leerLargoYMensaje(clientes[cliente].socket);
	enviarHeader(socketConsola, HeaderImprimirTextoConsola);
	enviarLargoYString(socketConsola, texto);
	free(texto);
}
void entradaSalida(int cliente) {
	char* serialIO = leerLargoYMensaje(clientes[cliente].socket);
	bloquearProcesoIO(clientes[cliente].pid,serialIO);
	// TODO Leer un int extra que es el tiempo de uso
	free(serialIO);
}
