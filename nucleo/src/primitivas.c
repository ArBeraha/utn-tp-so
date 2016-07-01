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
	log_info(activeLogger,"Wait para el semaforo %s",semid);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (sem->valor > 0){
		log_info(activeLogger,"Semaforo %s tiene carga %d y CPU%d continua",semid, sem->valor,cliente);
		sem->valor--;
	}
	else{
		bloquearProcesoSem(cliente, semid);}
	clientes[cliente].atentido=false;
}
void primitivaSignal(int cliente, char* semid) {
	log_info(activeLogger,"Signal para el semaforo %s",semid);
	t_semaforo* sem = (t_semaforo*) dictionary_get(tablaSEM, semid);
	if (!queue_is_empty(sem->cola)){
		t_proceso* proceso = queue_pop(sem->cola);
		desbloquearProceso(proceso);
		//planificarExpulsion(proceso);
	}
	else{
	sem->valor++;
	log_info(activeLogger,"Semaforo %s:%d",semid,sem->valor);
	}
	clientes[cliente].atentido=false;
}
void recibirAsignarCompartida(int cliente){
	char* compartidaSerial = leerLargoYMensaje(clientes[cliente].socket);
	char* compartida=string_from_format("!%s",compartidaSerial);
	char* valor = malloc(sizeof(int));
	read(clientes[cliente].socket, valor, sizeof(int));
	primitivaAsignarCompartida(compartida,char4ToInt(valor));
	t_proceso* proceso = obtenerProceso(cliente);
	if (!dictionary_has_key(tablaGlobales,compartida))
		proceso->abortado=true;
	free(compartida);
	free(compartidaSerial);
	free(valor);
	clientes[cliente].atentido=false;
}
void recibirDevolverCompartida(int cliente){
	char* compartidaSerial = leerLargoYMensaje(clientes[cliente].socket);
	char* compartida=string_from_format("!%s",compartidaSerial);
	char* valor = intToChar4(primitivaDevolverCompartida(compartida));
	send_w(clientes[cliente].socket, valor, sizeof(int));
	t_proceso* proceso = obtenerProceso(cliente);
	if (!dictionary_has_key(tablaGlobales,compartida))
		proceso->abortado=true;
	free(valor);
	free(compartidaSerial);
	free(compartida);
	clientes[cliente].atentido=false;
}
void primitivaAsignarCompartida(char* compartida, int valor) {
	if (dictionary_has_key(tablaGlobales,compartida))
		*(int*) dictionary_get(tablaGlobales, compartida) = valor;
	else
		log_error(activeLogger,"Se pidio asignar la compartida %s inexistente",compartida);
}
int primitivaDevolverCompartida(char* compartida) {
	if (dictionary_has_key(tablaGlobales,compartida))
		return (*(int*) dictionary_get(tablaGlobales, compartida));
	else
		log_error(activeLogger,"Se pidio el valor de la compartida %s inexistente",compartida);
	return 0;
}
void imprimirVariable(int cliente) {
	t_proceso* proceso = obtenerProceso(cliente);
	char* serialValor = malloc( sizeof(int));
	read(proceso->socketCPU, serialValor, sizeof(int));
	if (estaConectadoV2(clientes[proceso->consola])){
		enviarHeader(proceso->socketConsola, HeaderImprimirVariableConsola);
		send_w(proceso->socketConsola, serialValor, sizeof(ansisop_var_t));
	}
	free(serialValor);
	clientes[cliente].atentido=false;
}
void imprimirTexto(int cliente) {
	t_proceso* proceso = obtenerProceso(cliente);
	char* texto = leerLargoYMensaje(proceso->socketCPU);
	if (estaConectadoV2(clientes[proceso->consola])){
		enviarHeader(proceso->socketConsola, HeaderImprimirTextoConsola);
		enviarLargoYString(proceso->socketConsola, texto);
	}
	clientes[cliente].atentido=false;
	free(texto);
}
void entradaSalida(int cliente) {
	char* serialIO = leerLargoYMensaje(clientes[cliente].socket);
	char* serialTiempo = malloc(sizeof(int));
	read(clientes[cliente].socket,serialTiempo,sizeof(int));
	int tiempo = char4ToInt(serialTiempo);
	log_info(activeLogger,"CPU:%d pidio IO:%s por %d unidades de tiempo",cliente,serialIO,tiempo);
	bloquearProcesoIO(cliente,serialIO,tiempo);
	free(serialIO);
	free(serialTiempo);
	clientes[cliente].atentido=false;
}
