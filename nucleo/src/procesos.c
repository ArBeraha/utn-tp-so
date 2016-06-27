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
HILO crearProceso(int consola) {
	bool exploto = false;
	t_proceso* proceso = malloc(sizeof(t_proceso));
	proceso->PCB = pcb_create();
	proceso->abortado=false;
	proceso->PCB->PID = procesos++;
	proceso->estado = NEW;
	proceso->consola = consola;
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
		if (!pedirPaginas(proceso, codigo) || proceso->abortado) {
			rechazarProceso(proceso);
		} else {
			asignarMetadataProceso(proceso, codigo);
			MUTEXCLIENTES(clientes[consola].pid = consola);
			pcb_main(proceso->PCB);
			proceso->cpu = SIN_ASIGNAR;
			pthread_mutex_lock(&mutexPlanificacion);
//			MUTEXPROCESOS(list_add(listaProcesos, proceso));
			cambiarEstado(proceso, READY);
			pthread_mutex_unlock(&mutexPlanificacion);
			log_info(activeLogger,ANSI_COLOR_GREEN "Creado PID:%d" ANSI_COLOR_RESET,consola);
		}
		free(codigo);
	}
	return proceso;
}

void finalizarProceso(int cliente) {
	printf("ANTES DE OBTENER\n");
	t_proceso* proceso = obtenerProceso(cliente);
	log_info(activeLogger,ANSI_COLOR_RED "Finalizando PID:%d" ANSI_COLOR_RESET,proceso->PCB->PID);
	printf("DESP DE OBTENER\n");
	printf("ANTES DE CAMBIAR ESTADO\n");
	cambiarEstado(proceso,EXIT);
	printf("DESPUES DE CAMBIAR ESTADO\n");
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
void bloquearProcesoIO(int PID, char* IO, int tiempo) {
	if (dictionary_has_key(tablaIO, IO)) {
		log_info(activeLogger, "Añadiendo el Proceso a la cola del IO");
		bloquearProceso(PID);
		t_bloqueo* info = malloc(sizeof(t_bloqueo));
		info->IO = (t_IO*) dictionary_get(tablaIO, IO);
		info->PID = PID;
		info->tiempo = tiempo;
		queue_push((info->IO)->cola,(t_bloqueo*) info);
	} else
		log_info(activeLogger, "El IO solicitado no existe");
}
void bloquearProcesoSem(int PID, char* semid) {
	if (dictionary_has_key(tablaSEM, semid)) {
		log_info(activeLogger, "Añadiendo el Proceso a la cola del Semaforo");
		bloquearProceso(PID);
		queue_push(((t_semaforo*) dictionary_get(tablaSEM, semid))->cola,
				(t_proceso*) PID);
	} else
		log_info(activeLogger, "El Semaforo solicitado no existe");
}
void bloquearProceso(int PID) {
	log_info(activeLogger,"Bloqueando proceso pid:%d",PID);
	t_proceso* proceso = obtenerProceso(PID);
	cambiarEstado(proceso,BLOCK);
}
void desbloquearProceso(int PID) {
	log_info(activeLogger,"Desbloqueando proceso pid:%d",PID);
	t_proceso* proceso = obtenerProceso(PID);
	cambiarEstado(proceso,READY);
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
			printf("Etiqueta:%s Salto: %d\n",etiqueta,*salto);
			dictionary_put(p->PCB->indice_etiquetas, etiqueta, salto);
			i += sizeof(int);
			longitud = 0;
		} else
			longitud++;
	}
	free(metadata);
}
