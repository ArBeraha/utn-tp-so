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
HILO planificar() {
	while (1) {
		//Planificacion de Ejecucion y Destruccion de procesos
		planificacionFIFO();

		//Planificacion IO
		dictionary_iterator(tablaIO, (void*) planificarIO);
	}
}
void planificarExpulsion(t_proceso* proceso) {
	// mutexProcesos SAFE
	if (proceso->estado == EXEC) {
		if (terminoQuantum(proceso))
			expulsarProceso(proceso);
		else
			continuarProceso(proceso);
	}
}
void rafagaProceso(cliente){
	// mutexClientes SAFE
	log_info(activeLogger,"EL PID TERMINO UNA INSTRUCCION");
	t_proceso* proceso = (t_proceso*) clientes[cliente].pid;
	proceso->rafagas++;
	planificarExpulsion(proceso);
}
void planificacionFIFO() {
	// mutexProcesos SAFE
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
		crearHiloConParametro(&hiloBloqueos, (HILO)bloqueo, info);
	}
}
bool terminoQuantum(t_proceso* proceso) {
	// mutexProcesos SAFE
	return (proceso->rafagas>=config.quantum);
}
void asignarCPU(t_proceso* proceso, int cpu) {
	log_debug(bgLogger, "Asignando cpu:%d a pid:%d", cpu, proceso->PCB->PID);
	cambiarEstado(proceso,EXEC);
	proceso->cpu = cpu;
	proceso->rafagas=0;
	MUTEXCLIENTES(clientes[cpu].pid = proceso->PCB->PID);
	MUTEXCLIENTES(proceso->socketCPU = clientes[cpu].socket);
}
void desasignarCPU(t_proceso* proceso) {
	log_debug(bgLogger, "Desasignando cpu:%d a pid:%d", proceso->cpu,
			proceso->PCB->PID);
	queue_push(colaCPU, (void*) proceso->cpu);
	proceso->cpu = SIN_ASIGNAR;
	MUTEXCLIENTES(clientes[proceso->cpu].pid = (int) NULL);
}
void ejecutarProceso(int PID, int cpu) {
	// mutexProcesos SAFE
	t_proceso* proceso = (t_proceso*) PID;
	asignarCPU(proceso,cpu);
	if (!CU_is_test_running()) {
		int bytes = bytes_PCB(proceso->PCB);
		char* serialPCB = malloc(bytes);
		serializar_PCB(serialPCB, proceso->PCB);
		enviarHeader(proceso->socketCPU,HeaderPCB);
		enviarLargoYSerial(proceso->socketCPU, bytes, serialPCB);
		enviarHeader(proceso->socketCPU, HeaderContinuarProceso);
		free(serialPCB);
	}
}
void expulsarProceso(t_proceso* proceso) {
	// mutexProcesos SAFE
	enviarHeader(proceso->socketCPU, HeaderDesalojarProceso);
	cambiarEstado(proceso, READY);
	char* serialPcb = leerLargoYMensaje(proceso->socketCPU);
	pcb_destroy(proceso->PCB);
	t_PCB* pcb = malloc(sizeof(t_PCB));
	deserializar_PCB(pcb,serialPcb);
	proceso->PCB = pcb;
	// TODO usar actualizarPCB
}
void continuarProceso(t_proceso* proceso) {
	// mutexProcesos SAFE
	enviarHeader(proceso->socketCPU, HeaderContinuarProceso);
}
HILO bloqueo(t_bloqueo* info) {
	log_debug(bgLogger, "Ejecutando IO pid:%d por:%dseg", info->PID,
			info->IO->retardo);
	sleep(info->IO->retardo);
	desbloquearProceso(info->PID);
	info->IO->estado = INACTIVE;
	free(info);
	return NULL;
	}
void cambiarEstado(t_proceso* proceso, int estado) {
	bool legalidad;
	MUTEXESTADOS(legalidad = matrizEstados[proceso->estado][estado]);
	if (legalidad) {
		log_debug(bgLogger, "Cambio de estado pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
		if (proceso->estado == EXEC)
			desasignarCPU(proceso);
		if (estado == READY)
			queue_push(colaListos, proceso);
		else if (estado == EXIT)
			queue_push(colaSalida, proceso);
		proceso->estado = estado;
	} else
		log_error(activeLogger, "Cambio de estado ILEGAL pid:%d de:%d a:%d",
				proceso->PCB->PID, proceso->estado, estado);
}

