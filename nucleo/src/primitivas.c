/*
 * primitivas.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include "nucleo.h"

void recibirWait(int cliente){
	char* semid = leerLargoYMensaje(clientes[cliente].socket);
	primitivaWait(cliente,semid);
	free(semid);
}
void recibirSignal(int cliente){
	char* semid = leerLargoYMensaje(clientes[cliente].socket);
	primitivaSignal(cliente,semid);
	free(semid);
}
void primitivaWait(int cliente, char* semid) {
	log_info(activeLogger,"Llego un wait para el semaforo %s",semid);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (sem->valor > 0)
		sem->valor--;
	else
		bloquearProcesoSem(clientes[cliente].pid, semid);
}
void primitivaSignal(int cliente, char* semid) {
	log_info(activeLogger,"Llego un signal para el semaforo %s",semid);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (!queue_is_empty(sem->cola))
		desbloquearProceso((int)queue_pop(sem->cola));
	else
	sem->valor++;
}
void recibirAsignarCompartida(int cliente){
	char* compartida = leerLargoYMensaje(clientes[cliente].socket);
	char* valor = malloc(sizeof(int));
	read(clientes[cliente].socket, valor, sizeof(int));
	primitivaAsignarCompartida(compartida,char4ToInt(valor));
	free(compartida);
	free(valor);
}
void recibirDevolverCompartida(int cliente){
	char* compartida = leerLargoYMensaje(clientes[cliente].socket);
	char* valor = intToChar4(primitivaDevolverCompartida(compartida));
	send_w(clientes[cliente].socket, valor, sizeof(int));
	free(valor);
	free(compartida);
}
void primitivaAsignarCompartida(char* compartida, int valor) {
	*(int*) dictionary_get(tablaGlobales, compartida) = valor;
}
int primitivaDevolverCompartida(char* compartida) {
	if (dictionary_has_key(tablaGlobales,compartida))
		return (*(int*) dictionary_get(tablaGlobales, compartida));
	else
		log_error(activeLogger,"Se pidio el valor de la global %s inexistente",compartida);
	return 0;
}
void imprimirVariable(int cliente) {
	printf("Imprimir Variable de CPU:%d\n",cliente);
	int socketConsola = procesos[clientes[cliente].pid]->socketConsola;
	printf("Obtenido socketConsola:%d\n",socketConsola);
	char* serialValor = malloc( sizeof(int));
	read(clientes[cliente].socket, serialValor, sizeof(int));
	enviarHeader(socketConsola, HeaderImprimirVariableConsola);
	send_w(socketConsola, serialValor, sizeof(ansisop_var_t));
	free(serialValor);
	clientes[cliente].atentido=false;
}
void imprimirTexto(int cliente) {
	int socketConsola = procesos[clientes[cliente].pid]->socketConsola;
	char* texto = leerLargoYMensaje(clientes[cliente].socket);
	enviarHeader(socketConsola, HeaderImprimirTextoConsola);
	enviarLargoYString(socketConsola, texto);
	clientes[cliente].atentido=false;
	free(texto);
}
void entradaSalida(int cliente) {
	log_info(activeLogger,"Proceso pidio IO");
	char* serialIO = leerLargoYMensaje(clientes[cliente].socket);
	char* serialTiempo = malloc(sizeof(int));
	read(clientes[cliente].socket,serialTiempo,sizeof(int));
	bloquearProcesoIO(clientes[cliente].pid,serialIO,char4ToInt(serialTiempo));
	free(serialIO);
	free(serialTiempo);
}
