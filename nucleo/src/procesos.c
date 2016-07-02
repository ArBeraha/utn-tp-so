/*
 * procesos.c
 *
 *  Created on: 25/5/2016
 *      Author: utnso
 */
#include <parser/metadata_program.h>
#include <math.h>
#include "nucleo.h"

t_proceso* obtenerProceso(int cliente){
	t_proceso* proceso = clientes[cliente].proceso;
	if (proceso == NULL){
		log_error(activeLogger,"Se solicita el proceso del cliente:%d inexistente", cliente);
		return NULL;}
	else
	return proceso;
}

/*  ----------INICIO PROCESOS---------- */
void rechazarProceso(t_proceso* proceso) {
	if (proceso->estado != NEW)
		log_warning(activeLogger,
				"Se esta rechazando el proceso %d ya aceptado!", proceso->PCB->PID);
	enviarHeader(proceso->socketConsola, HeaderConsolaFinalizarRechazado);
	log_info(bgLogger,
			"Consola avisada sobre la finalización del proceso ansisop.");
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
//	MUTEXPROCESOS(list_remove_by_value(listaProcesos, (void*) PID));
	// POSIBLE DOBLE ELIMINACION?
//	pthread_mutex_lock(&mutexClientes);
//	quitarCliente(proceso->consola);
//	pthread_mutex_unlock(&mutexClientes);
//	MUTEXPROCESOS(procesos[PID] = NULL);
	pcb_destroy(proceso->PCB);

	free(proceso);
}
void liberarRecursos(t_proceso* proceso){
	enviarHeader(umc,HeaderLiberarRecursosPagina);
		char* serialPID = intToChar4(proceso->PCB->PID);
		send_w(umc,serialPID,sizeof(int));
}


HILO crearProceso(int consola) {
	bool exploto = false;
	t_proceso* proceso = malloc(sizeof(t_proceso));
	proceso->PCB = pcb_create();
	proceso->abortado=false;
	proceso->PCB->PID = procesos++;
	proceso->estado = NEW;
	proceso->consola = consola;
	proceso->sigusr1=false;
	MUTEXCLIENTES(clientes[consola].proceso=proceso);
	MUTEXCLIENTES(proceso->socketConsola = clientes[consola].socket);
	log_info(bgLogger,"Se esta intentando iniciar el proceso PID:%d",proceso->PCB->PID);
	if (!CU_is_test_running()) {
		char* codigo = getScript(consola,&exploto);
		if (exploto){
			log_info(activeLogger,ANSI_COLOR_RED "[FLAG RECUPERACION]" ANSI_COLOR_RESET,consola);
			pcb_destroy(proceso->PCB);
			free(proceso);
			return 0;
		}
		proceso->PCB->cantidad_paginas = ceil(
				((double) strlen(codigo)) / ((double) tamanio_pagina));
		if (!pedirPaginas(proceso, codigo)) {
			rechazarProceso(proceso);
		} else {
			if (proceso->abortado){
				liberarRecursos(proceso);
				finalizarProceso(consola);
				return 0;
			}
			asignarMetadataProceso(proceso, codigo);
			MUTEXCLIENTES(clientes[consola].pid = consola);
			pcb_main(proceso->PCB);
			proceso->cpu = SIN_ASIGNAR;
			pthread_mutex_lock(&mutexPlanificacion);
//			MUTEXPROCESOS(list_add(listaProcesos, proceso));
			cambiarEstado(proceso, READY);
			pthread_mutex_unlock(&mutexPlanificacion);
			log_info(activeLogger,ANSI_COLOR_GREEN "Creado PID:%d para la consola %d." ANSI_COLOR_RESET,proceso->PCB->PID,consola);
		}
		free(codigo);
	}
	return proceso;
}

void finalizarProceso(int cliente) {
	t_proceso* proceso = obtenerProceso(cliente);
	log_info(activeLogger,ANSI_COLOR_RED "Finalizando PID:%d" ANSI_COLOR_RESET,proceso->PCB->PID);
	cambiarEstado(proceso,EXIT);
	//MUTEXPROCESOS(list_remove_by_value(listaProcesos, (void*) PID));
}
void destruirProceso(t_proceso* proceso) {
	// mutexProcesos SAFE
	log_info(bgLogger,	"Destruyendo proceso:%d",proceso->PCB->PID);
	if (proceso->estado != EXIT)
		log_warning(activeLogger,
				"Se esta destruyendo el proceso %d que no libero sus recursos! y esta en estado:%d",
				proceso->PCB->PID, proceso->estado);
	clientes[proceso->consola].proceso=NULL;
	if (!CU_is_test_running()) {
		if (proceso->abortado)
			enviarHeader(proceso->socketConsola,
					HeaderConsolaFinalizarErrorInstruccion);
		else
			enviarHeader(proceso->socketConsola,
					HeaderConsolaFinalizarNormalmente);
		//MUTEXCLIENTES(quitarCliente(proceso->consola)); Doble Eliminacion
	}
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	//MUTEXPROCESOS(procesos[proceso->PCB->PID] = NULL);

	pcb_destroy(proceso->PCB);
	free(proceso); // Destruir Proceso y PCB

}
void actualizarPCB(t_proceso* proceso, t_PCB* PCB) { //
	pcb_destroy(proceso->PCB);
	proceso->PCB = PCB;
	//imprimir_PCB(proceso->PCB);
}
void ingresarCPU(int cliente){
	MUTEXPLANIFICACION(queue_push(colaCPU,(void*)cliente));
}
void bloquearProcesoIO(int cliente, char* IO, int tiempo) {
	t_proceso* proceso = obtenerProceso(cliente);
	if (dictionary_has_key(tablaIO, IO)) {
		log_info(activeLogger, "Añadiendo el Proceso a la cola del IO");

		bloquearProceso(proceso);
		t_bloqueo* info = malloc(sizeof(t_bloqueo));
		info->IO = (t_IO*) dictionary_get(tablaIO, IO);
		info->proceso = proceso;
		info->tiempo = tiempo;
		queue_push((info->IO)->cola,(t_bloqueo*) info);
	} else{
		log_error(activeLogger, "El IO solicitado no existe");
		proceso->abortado=true;
	}
	dictionary_iterator(tablaIO,imprimirColasIO);
}
void bloquearProcesoSem(int cliente, char* semid) {
	if (dictionary_has_key(tablaSEM, semid)) {
		log_info(activeLogger, "Añadiendo el Proceso a la cola del Semaforo");
		t_proceso* proceso = obtenerProceso(cliente);
		bloquearProceso(proceso);
		queue_push(((t_semaforo*) dictionary_get(tablaSEM, semid))->cola,
				proceso);
	} else
		log_error(activeLogger, "El Semaforo solicitado no existe");
	dictionary_iterator(tablaSEM,imprimirColasSemaforos);
}
void bloquearProceso(t_proceso* proceso) {
	log_info(activeLogger,"Bloqueando proceso pid:%d",proceso->PCB->PID);
	cambiarEstado(proceso,BLOCK);
}
void desbloquearProceso(t_proceso* proceso) {
	log_info(activeLogger,"Desbloqueando proceso pid:%d",proceso->PCB->PID);
	cambiarEstado(proceso,READY);
}
void asignarMetadataProceso(t_proceso* p, char* codigo) {
	int i;
	t_medatada_program* metadata = metadata_desde_literal(codigo);
	p->PCB->PC=metadata->instruccion_inicio;
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
			printf("Etiqueta:%s Salto: %d\n",etiqueta,*salto);
			dictionary_put(p->PCB->indice_etiquetas, etiqueta, salto);
			i += sizeof(int);
			longitud = 0;
		} else
			longitud++;
	}
	free(metadata);
}
