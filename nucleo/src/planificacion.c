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
int cantidadProcesos() {
	int cantidad;
	pthread_mutex_lock(&mutexProcesos);
	cantidad = list_size(listaProcesos);
	// El unlock se hace dos o tres lineas despues de llamar a esta funcion
	return cantidad;
}
void planificacionFIFO() {
	if (!queue_is_empty(colaListos) && !queue_is_empty(colaCPU))
		ejecutarProceso((int) queue_pop(colaListos), (int) queue_pop(colaCPU));

	if (!queue_is_empty(colaSalida))
		destruirProceso((int) queue_pop(colaSalida));
}
void planificacionRR() {
	int i;
	for (i = 0; i < cantidadProcesos(); i++) { // Con cantidadProcesos() se evitan condiciones de carrera.
		t_proceso* proceso = list_get(listaProcesos, i);
		pthread_mutex_unlock(&mutexProcesos); // El lock se hace en cantidadProcesos()
		if (proceso->estado == EXEC) {
			if (terminoQuantum(proceso))
				expulsarProceso(proceso);
		}
	}
	planificacionFIFO();
}
void planificarProcesos() {
	switch (algoritmo) {
	// Procesos especificos
	case RR:
		planificacionRR();
		break;
	case FIFO:
		planificacionFIFO();
		break;
	}

	//Planificar IO
	dictionary_iterator(tablaIO, (void*) planificarIO);
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
	} else
		log_error(activeLogger, "Cambio de estado ILEGAL pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
}

