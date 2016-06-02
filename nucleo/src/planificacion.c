/*
 * planificacion.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include "nucleo.h"

static bool matrizEstados[5][5] = {
//		     		NEW    READY  EXEC   BLOCK  EXIT
		/* NEW 	 */{ false, true, false, false, true },
		/* READY */{ false, false, true, false, true },
		/* EXEC  */{ false, true, false, true, true },
		/* BLOCK */{ false, true, false, false, true },
		/* EXIT  */{ false, false, false, false, false } };

/*  ----------INICIO PLANIFICACION ---------- */
void planificar() {
	while (1) {
		//Procesos
		planificarProcesos();
		//IO
		dictionary_iterator(tablaIO, (void*) planificarIO);
	}
}
void planificarProcesos() {
	switch (algoritmo) {

	case RR:
		planificacionRR();
		break;
	case FIFO:
		planificacionFIFO();
		break;
	}
}
void planificacionRR() {
	pthread_mutex_lock(&mutexProcesos);
	list_iterate(listaProcesos, (void*) planificarProcesoRR);
	pthread_mutex_unlock(&mutexProcesos);
	planificacionFIFO();
}
void planificarProcesoRR(t_proceso* proceso) {
	if (proceso->estado == EXEC) {
		if (terminoQuantum(proceso))
			expulsarProceso(proceso);
		else
			continuarProceso(proceso);
	}
}
void planificacionFIFO() {
	while (!queue_is_empty(colaListos) && !queue_is_empty(colaCPU))
		ejecutarProceso((int) queue_pop(colaListos), (int) queue_pop(colaCPU));

	while (!queue_is_empty(colaSalida))
		destruirProceso((int) queue_pop(colaSalida));
}
void planificarIO(char* io_id, t_IO* io) {
	if (io->estado == INACTIVE && (!queue_is_empty(io->cola))) {
		io->estado = ACTIVE;
		t_bloqueo* info = malloc(sizeof(t_bloqueo));
		info->IO = io;
		info->PID = (int) queue_pop(io->cola);
		pthread_create(&hiloBloqueos, &detachedAttr, (void*) bloqueo, info);
	}
}
bool terminoQuantum(t_proceso* proceso) {
	return (!(proceso->PCB->PC % config.quantum)); // Si el PC es divisible por QUANTUM quiere decir que hizo QUANTUM ciclos
}
void asignarCPU(t_proceso* proceso, int cpu) {
	log_debug(bgLogger, "Asignando cpu:%d a pid:%d", cpu, proceso->PCB->PID);
	proceso->cpu = cpu;
	clientes[cpu].pid = proceso->PCB->PID;

}
void desasignarCPU(t_proceso* proceso) {
	log_debug(bgLogger, "Desasignando cpu:%d a pid:%d", proceso->cpu,
			proceso->PCB->PID);
	queue_push(colaCPU, (void*) proceso->cpu);
	proceso->cpu = SIN_ASIGNAR;
	clientes[proceso->cpu].pid = (int) NULL;
}
void expulsarProceso(t_proceso* proceso) {
	cambiarEstado(proceso, READY);
	desasignarCPU(proceso);
	enviarHeader(proceso->cpu, HeaderDesalojarProceso);
}
void continuarProceso(t_proceso* proceso) {
	enviarHeader(proceso->cpu, HeaderContinuarProceso);
}
void bloqueo(t_bloqueo* info) {
	log_debug(bgLogger, "Ejecutando IO pid:%d por:%dseg", info->PID,
			info->IO->retardo);
	sleep(info->IO->retardo);
	desbloquearProceso(info->PID);
	info->IO->estado = INACTIVE;
	free(info);
}
void cambiarEstado(t_proceso* proceso, int estado) {
	if (matrizEstados[proceso->estado][estado]) {
		log_debug(bgLogger, "Cambio de estado pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
		proceso->estado = estado;
		if (estado==READY)
			queue_push(colaListos, proceso);
		else
		if (estado==EXIT)
			queue_push(colaSalida, proceso);
	} else
		log_error(activeLogger, "Cambio de estado ILEGAL pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
}

