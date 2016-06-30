/*
 * planificacion.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include "nucleo.h"


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

bool isclosed(int sock) {
  fd_set rfd;
  FD_ZERO(&rfd);
  FD_SET(sock, &rfd);
  struct timeval e;
  e.tv_sec = 0;
  e.tv_usec = 500000;

  select(sock+1, &rfd, 0, 0, &e);
  if (!FD_ISSET(sock, &rfd))
    return false;
  int n = 0;
  ioctl(sock, FIONREAD, &n);
  return n == 0;
}

static bool matrizEstados[5][5] = {
//		     		NEW    READY  EXEC   BLOCK  EXIT
		/* NEW 	 */{ false, true, false, false, true },
		/* READY */{ false, false, true, false, true },
		/* EXEC  */{ false, true, false, true, true },
		/* BLOCK */{ false, true, true, false, true },
		/* EXIT  */{ false, false, false, false, false } };

/*  ----------INICIO PLANIFICACION ---------- */
HILO planificar() {
	while (1) {
		//Planificacion de Ejecucion y Destruccion de procesos
		pthread_mutex_lock(&mutexPlanificacion);
		planificacionFIFO();
		pthread_mutex_unlock(&mutexPlanificacion);

		//Planificacion IO
		dictionary_iterator(tablaIO, (void*) planificarIO);
	}
}
void planificarExpulsion(t_proceso* proceso) {
	// mutexProcesos SAFE
	// mutexPlanificacion SAFE
	if (estaConectado(clientes[proceso->cpu])) {
		if (proceso->estado == BLOCK) {
			expulsarProceso(proceso);
			return;
		}

		if (proceso->estado == EXEC
				&& (terminoQuantum(proceso) || proceso->abortado))
			expulsarProceso(proceso);
		else
			continuarProceso(proceso);
	}
	if (proceso->abortado){
//		pthread_mutex_unlock(&mutexClientes);
		liberarRecursos(proceso);
		finalizarProceso(proceso->consola);
//		pthread_mutex_lock(&mutexClientes);
	}
}
void rafagaProceso(int cliente){
	// mutexClientes SAFE
	t_proceso* proceso = obtenerProceso(cliente);
	log_info(debugLogger,"EL PID:%d TERMINO UNA INSTRUCCION",proceso->PCB->PID);
	proceso->rafagas++;
	planificarExpulsion(proceso);
	clientes[cliente].atentido=false;
}
bool procesoExiste(t_proceso* proceso){
	int i;
	for (i=0;i<getMaxClients();i++){
		if (clientes[i].proceso==proceso)
			return true;
	}
	return false;
}

t_proceso* buscarPID(int PID){
	int i;
	for (i=0;i<getMaxClients();i++){
		if (clientes[i].pid==PID)
			return clientes[i].proceso;
	}
	return NULL;
}
bool clienteExiste(int cliente){
	if (clientes[cliente].socket != 0)
		return true;
	else
		return false;
}
void planificacionFIFO() {
	// mutexProcesos SAFE

	while (!queue_is_empty(colaSalida))
		destruirProceso(queue_pop(colaSalida));



	while (!queue_is_empty(colaListos) && !queue_is_empty(colaCPU)) {
		 //Limpiamos las colas de procesos eliminados hasta encontrar uno que no lo este o se vacie
		while (!queue_is_empty(colaListos)
				&& (!procesoExiste( (t_proceso*) queue_peek(colaListos)) || ( (t_proceso*) queue_peek(colaListos))->estado!=READY))
			queue_pop(colaListos);
		// Limpiamos las colas de clientes desconectados hasta encontrar uno que no lo este o se vacie
		while (!queue_is_empty(colaCPU) && (!clienteExiste( (int) queue_peek(colaCPU)) || !estaConectado(clientes[(int) queue_peek(colaCPU)])))
			queue_pop(colaCPU);

		// Si no se vaciaron las listas entonces los primeros de ambas listas son validos
		if (!queue_is_empty(colaListos) && !queue_is_empty(colaCPU)){
			log_info(activeLogger,"[Planificando] Disponibles PID y CPU");
			ejecutarProceso((t_proceso*) queue_pop(colaListos),
					(int) queue_pop(colaCPU));}

		// Si por lo menos una lista no se vacio repetir el proceso
	}

}
void planificarIO(char* io_id, t_IO* io) {
	if (io->estado == INACTIVE && (!queue_is_empty(io->cola))) {
		io->estado = ACTIVE;
		crearHiloConParametro(&io->hilo, (HILO)bloqueo, queue_pop(io->cola));
	}
}
bool terminoQuantum(t_proceso* proceso) {
	// mutexProcesos SAFE
	log_info(debugLogger,"PREGUNTANDO POR QUANTUM");
	return (proceso->rafagas>=config.quantum);
}
void asignarCPU(t_proceso* proceso, int cpu) {
	log_info(bgLogger, "Asignando cpu:%d a pid:%d", cpu, proceso->PCB->PID);
	cambiarEstado(proceso,EXEC);
	proceso->cpu = cpu;
	proceso->rafagas=0;
	MUTEXCLIENTES(clientes[cpu].proceso = proceso);
	MUTEXCLIENTES(clientes[cpu].pid = proceso->PCB->PID);
	MUTEXCLIENTES(proceso->socketCPU = clientes[cpu].socket);
}
void desasignarCPU(t_proceso* proceso) {
	log_info(bgLogger, "Desasignando cpu:%d a pid:%d", proceso->cpu,
			proceso->PCB->PID);
	queue_push(colaCPU, (void*) proceso->cpu);
	proceso->cpu = SIN_ASIGNAR;
	clientes[proceso->cpu].proceso = NULL;
	clientes[proceso->cpu].pid = -1;
}
void ejecutarProceso(t_proceso* proceso, int cpu) {
	// mutexProcesos SAFE
	log_info(activeLogger,ANSI_COLOR_GREEN "Ejecutando PID:%d con CPU:%d" ANSI_COLOR_RESET, proceso->PCB->PID, cpu);
	asignarCPU(proceso,cpu);
	if (!CU_is_test_running()) {
		int bytes = bytes_PCB(proceso->PCB);
		char* serialPCB = malloc(bytes);
		serializar_PCB(serialPCB, proceso->PCB);
		if (!isclosed(proceso->socketCPU)) {
			enviarHeader(proceso->socketCPU,HeaderPCB);
			enviarLargoYSerial(proceso->socketCPU, bytes, serialPCB);
			continuarProceso(proceso);
		}
		free(serialPCB);
	}
}
void expulsarProceso(t_proceso* proceso) {
	// mutexProcesos SAFE
	log_info(activeLogger,ANSI_COLOR_RED "Expulsando PID:%d de CPU:%d" ANSI_COLOR_RESET,proceso->PCB->PID,proceso->cpu);
	enviarHeader(proceso->socketCPU, HeaderDesalojarProceso);
	char* serialPcb = leerLargoYMensaje(proceso->socketCPU);
	t_PCB* pcb = malloc(sizeof(t_PCB));
	deserializar_PCB(pcb,serialPcb);
	actualizarPCB(proceso,pcb);
	if (!proceso->abortado && proceso->estado==EXEC){
		cambiarEstado(proceso, READY);
	}
	free(serialPcb);
	desasignarCPU(proceso);
}

void continuarProceso(t_proceso* proceso) {
	// mutexProcesos SAFE
	log_info(activeLogger,"Continuando PID:%d",proceso->PCB->PID);
	char* serialSleep = intToChar4(config.queantum_sleep);
	if (!isclosed(proceso->socketCPU)) {
		enviarHeader(proceso->socketCPU, HeaderContinuarProceso);
		send_w(proceso->socketCPU, serialSleep, sizeof(int));
	}
	else
		printf("MUERTE POR DESC EN CONT\n\n");
	free(serialSleep);
}
HILO bloqueo(t_bloqueo* info) {
	log_info(bgLogger, "Ejecutando IO pid:%d por:%dseg", info->proceso->PCB->PID,
			info->IO->retardo*info->tiempo);
	usleep(info->IO->retardo*info->tiempo);
	desbloquearProceso(info->proceso);
	info->IO->estado = INACTIVE;
	free(info);
	return NULL;
	}
void cambiarEstado(t_proceso* proceso, int estado) {
	bool legalidad;
	MUTEXESTADOS(legalidad = matrizEstados[proceso->estado][estado]);
	if (legalidad) {
		log_info(bgLogger, "Cambio de estado pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
		if (estado == READY){
			log_info(activeLogger,"Añadiendo a colaListos pid:%d",proceso->PCB->PID);
			queue_push(colaListos, proceso);}
		else if (estado == EXIT){
			log_info(activeLogger,"Añadiendo a colaSalida pid:%d",proceso->PCB->PID);
			queue_push(colaSalida, proceso);}
		proceso->estado = estado;
	} else{
		log_error(activeLogger, "Cambio de estado ILEGAL pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
		exit(EXIT_FAILURE);
	}
}

