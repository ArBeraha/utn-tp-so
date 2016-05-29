/*
 * procesos.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include <parser/metadata_program.h>
#include <math.h>
#include "nucleo.h"
int tamanio_pagina = 20; // TODO obtenerlo despues del handshake

/*  ----------INICIO PROCESOS---------- */
void rechazarProceso(int PID) {
	t_proceso* proceso = (t_proceso*) PID;
	if (proceso->estado != NEW)
		log_warning(activeLogger,
				"Se esta rechazando el proceso %d ya aceptado!", PID);
	send(clientes[proceso->consola].socket,
			intToChar(HeaderConsolaFinalizarRechazado), 1, 0); // Le decimos adios a la consola
	log_info(bgLogger,
			"Consola avisada sobre la finalización del proceso ansisop.");
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	pthread_mutex_lock(&lockProccessList);
	list_remove_by_value(listaProcesos, (void*) PID);
	pthread_mutex_unlock(&lockProccessList);
	pcb_destroy(proceso->PCB);
	free(proceso);
}
int crearProceso(int consola) {
	t_proceso* proceso = malloc(sizeof(t_proceso));
	proceso->PCB = pcb_create();
	proceso->PCB->PID = (int) proceso;
	proceso->estado = NEW;
	proceso->consola = consola;
	clientes[consola].pid = proceso->PCB->PID;
	proceso->cpu = SIN_ASIGNAR;
	pthread_mutex_lock(&lockProccessList);
	list_add(listaProcesos, proceso);
	pthread_mutex_unlock(&lockProccessList);
	if (!CU_is_test_running()) {
		char* codigo = getScript(consola);
		asignarMetadataProceso(proceso, codigo);
		proceso->PCB->cantidad_paginas = ceil(
				((double) strlen(codigo)) / ((double) tamanio_pagina));
		if (pedirPaginas(proceso->PCB->PID, codigo))
			cambiarEstado(proceso,READY);
		else
			rechazarProceso(proceso->PCB->PID);
		free(codigo);
	}
	return proceso->PCB->PID;
}
void cargarProceso(int consola) {
	// Crea un hilo que crea el proceso y se banca esperar a que umc le de paginas.
	// Mientras tanto, el planificador sigue andando.
	pthread_create(&crearProcesos, &detachedAttr, (void*) crearProceso,
			(void*) consola);
}
void ejecutarProceso(int PID, int cpu) {
	t_proceso* proceso = (t_proceso*) PID;
	cambiarEstado(proceso,EXEC);
	asignarCPU(proceso,cpu);
	int bytes = bytes_PCB(proceso->PCB);
	char* serialPCB = malloc(bytes);
	char* serialBytes = intToChar4(bytes);
	serializar_PCB(serialPCB, proceso->PCB);
	send_w(clientes[cpu].socket, "HEADER MANDAR A EJECUTAR", 1); //TODO header
	send_w(clientes[cpu].socket, serialBytes, sizeof(int));
	send_w(clientes[cpu].socket, serialPCB, bytes);
	free(serialPCB);
	free(serialBytes);
}
void finalizarProceso(int PID) {
	t_proceso* proceso = (t_proceso*) PID;
	if (proceso->estado==EXEC)
	desasignarCPU(proceso);
	cambiarEstado(proceso,EXIT);
	queue_push(colaSalida, (void*) PID);
	pthread_mutex_lock(&lockProccessList);
	list_remove_by_value(listaProcesos, (void*) PID);
	pthread_mutex_unlock(&lockProccessList);
}
void destruirProceso(int PID) {
	log_debug(bgLogger,	"Destruyendo proceso:%d",PID);
	t_proceso* proceso = (t_proceso*) PID;
	if (proceso->estado != EXIT)
		log_warning(activeLogger,
				"Se esta destruyendo el proceso %d que no libero sus recursos! y esta en estado:%d",
				PID, proceso->estado);
	char* serialHeader = intToChar(HeaderConsolaFinalizarNormalmente);
	send(clientes[proceso->consola].socket, serialHeader, 1, 0); // Le decimos adios a la consola
	quitarCliente(proceso->consola); // Esto no es necesario, ya que si la consola funciona bien se desconectaria, pero quien sabe...
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	pcb_destroy(proceso->PCB);
	free(proceso); // Destruir Proceso y PCB
	free(serialHeader);
}
void actualizarPCB(t_PCB PCB) { //
	// Cuando CPU me actualice la PCB del proceso me manda una PCB (no un puntero)
	pthread_mutex_lock(&lockProccessList);
	//t_proceso* proceso = list_get(listaProcesos, PCB->PID);
	pthread_mutex_unlock(&lockProccessList);
	//proceso->PCB=PCB;
}
void expulsarProceso(t_proceso* proceso) {
	cambiarEstado(proceso,READY);
	queue_push(colaListos, (void*) proceso->PCB->PID);
	desasignarCPU(proceso);
}
void bloquearProcesoIO(int PID, char* IO) {
	bloquearProceso(PID);
	if (dictionary_has_key(tablaIO, IO))
		queue_push(((t_IO*) dictionary_get(tablaIO, IO))->cola,
				(t_proceso*) PID);
}
void bloquearProcesoSem(int PID, char* semid) {
	bloquearProceso(PID);
	if (dictionary_has_key(tablaSEM, semid))
		queue_push(((t_semaforo*) dictionary_get(tablaSEM, semid))->cola,
				(t_proceso*) PID);
}
void bloquearProceso(int PID) {
	t_proceso* proceso = (t_proceso*) PID;
	cambiarEstado(proceso,BLOCK);
	desasignarCPU(proceso);
}
void desbloquearProceso(int PID) {
	t_proceso* proceso = (t_proceso*) PID;
	cambiarEstado(proceso,READY);
	queue_push(colaListos, (t_proceso*) PID);
}
void asignarMetadataProceso(t_proceso* p, char* codigo) {
	int i;
	t_medatada_program* metadata = metadata_desde_literal(codigo);
	t_sentencia* sentencia;
	for (i = 0; i < metadata->instrucciones_size; i++) {
		sentencia = malloc(sizeof(t_sentencia));
		sentencia->offset_inicio = metadata->instrucciones_serializado[i].start;
		sentencia->offset_fin = sentencia->offset_inicio
				+ metadata->instrucciones_serializado[i].offset;
		list_add(p->PCB->indice_codigo, sentencia);
	}
	int longitud = 0;
	for (i = 0; i < metadata->etiquetas_size; i++) {
		if (metadata->etiquetas[i] == '\0') {
			char* etiqueta = malloc(longitud + 1);
			memcpy(etiqueta, metadata->etiquetas + i - longitud, longitud + 1);
			int* salto = malloc(sizeof(int));
			memcpy(salto, metadata->etiquetas + i + 1, sizeof(int));
			//imprimir_serializacion(etiqueta,longitud);
			//printf("Etiqueta:%s Salto: %d\n",etiqueta,*salto);
			dictionary_put(p->PCB->indice_etiquetas, etiqueta, salto);
			i += sizeof(int);
			longitud = 0;
		} else
			longitud++;
	}
	free(metadata);
}
