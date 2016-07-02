/*
 * consolaUmc.c
 *
 *  Created on: 19/6/2016
 *      Author: utnso
 */
#include "umc.h"

//2. Funciones que se mandan por consola

HILO recibirComandos(){
	int funcion;
	do {
		char* selecc = NULL;
		size_t bufsize = 64;

		printf(" \n \n");
		printf("Funciones: 0.salir / 1.retardo / 2.dumpEstructuraMemoria / 3.dumpContenidoMemoria / 4.flushTlb / 5.flushMemory / 6.MostrarTLB / 7.TodasPagsEnMemoria\n");
		printf("Funcion: ");

		getline(&selecc,&bufsize,stdin);
		funcion = atoi(selecc);

		switch(funcion){
			case 0: break;
			case 1: fRetardo(); break;
			case 2: dumpEstructuraMemoria();break;
			case 3: dumpContenidoMemoria();break;
			case 4: flushTlb();break;
			case 5: flushMemory();break;
			case 6: mostrarTlb();break;
			case 7: devolverTodasLasPaginas(0);
			default: break;
		}
	}
	while(funcion!=9);
	return 0;
}

void flushTlb(){
	inicializarTlb();
	mostrarTlb();
}
void flushMemory(){ //Pone a todas las paginas bit de modificacion en 1
	pthread_mutex_lock(&lock_accesoTabla);
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;
	for(i=0;i<cantidadTablas;i++){

		tabla_t* unaTabla = list_get(listaTablasPaginas,i);
		int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = list_get((t_list*)unaTabla->listaPaginas,j);
			unaPagina->bitModificacion=1;
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
	devolverTodasLasPaginas(0);
}

void devolverTodasLasPaginas(int soloPresencia){  //OK
	pthread_mutex_lock(&lock_accesoTabla);
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		tabla_t* unaTabla = malloc(sizeof(tabla_t));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
		int j;
		log_info(dump, "TODAS LAS PAGINAS \n");

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get((t_list*)unaTabla->listaPaginas,j);
			if(unaPagina->bitPresencia && soloPresencia){
				printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitUso: %d, bitModificacion: %d \n",unaTabla->pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitUso,unaPagina->bitModificacion);
				log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitUso: %d, bitModificacion: %d ",unaTabla->pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitUso,unaPagina->bitModificacion);
			}
			if(soloPresencia==0){
				printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitUso: %d, bitModificacion: %d \n",unaTabla->pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitUso,unaPagina->bitModificacion);
				log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitUso: %d, bitModificacion: %d ",unaTabla->pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitUso,unaPagina->bitModificacion);
			}
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
	log_info(dump, "------------------------------- \n");
}

void devolverPaginasDePid(int pid){ //OK
	tabla_t* unaTabla = malloc(sizeof(tabla_t));

	if(existePidEnListadeTablas(pid)){

			pthread_mutex_lock(&lock_accesoTabla);
			unaTabla = buscarTabla(pid);

			int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
			int i;
			log_info(dump, "PAGINAS DE PID: %d \n", pid);

			for(i=0;i<cantidadPaginasDeTabla;i++){
				tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
				unaPagina = list_get((t_list*)unaTabla->listaPaginas,i);
				if(unaPagina->bitPresencia){
					printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
					log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
				}
			}
			log_info(dump, "------------------------------- \n");
	}else{
		printf("Ese pid no existe o no esta en uso! \n");
	}
	pthread_mutex_unlock(&lock_accesoTabla);
}



void devolverTodaLaMemoria(){
	pthread_mutex_lock(&lock_accesoTabla);
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		tabla_t* unaTabla = malloc(sizeof(t_list));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
		int j;
		log_info(dump,"TODA LA MEMORIA \n");

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get((t_list*)unaTabla->listaPaginas,j);
			//Hago un solo print f de las caracteristicas


			if(unaPagina->bitPresencia==1){
				printf("\nPid: %d, Pag: %d, Marco: %d, Contenido: ",unaTabla->pid, unaPagina->nroPagina,unaPagina->marcoUtilizado);
				log_info(dump,"\nPid: %d, Pag: %d, Marco: %d, Contenido: ",unaTabla->pid, unaPagina->nroPagina,unaPagina->marcoUtilizado);
				pthread_mutex_lock(&lock_accesoMemoria);
				char* contenido = malloc(config.tamanio_marco);
				memcpy(contenido,memoria+(unaPagina->marcoUtilizado*config.tamanio_marco),config.tamanio_marco);
				pthread_mutex_unlock(&lock_accesoMemoria);

				if(j<cantPaginasDePid(unaTabla->pid)-paginas_stack){
					imprimirRegionMemoriaCodigoLogDump(contenido,config.tamanio_marco);
					imprimirRegionMemoriaCodigoConsola(contenido,config.tamanio_marco);
				}else{
					imprimirRegionMemoriaStackLogDump(contenido,config.tamanio_marco);
					imprimirRegionMemoriaStackConsola(contenido,config.tamanio_marco);
				}
			}
//			else{
//				printf("No esta en memoria");
//				log_info(dump,"No esta en memoria",unaTabla->pid, unaPagina->nroPagina,unaPagina->marcoUtilizado);
//			}
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
	log_info(dump, "------------------------------- \n");
	printf("\n");
}

void devolverMemoriaDePid(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* unaTabla = malloc(sizeof(tabla_t));

	unaTabla = buscarTabla(pid);
	int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
	int i;
	log_info(dump,"MEMORIA DE PID: %d \n",pid);

	for(i=0;i<cantidadPaginasDeTabla;i++){
		tablaPagina_t* unaPagina;
		unaPagina = list_get((t_list*)unaTabla->listaPaginas,i);

		if(unaPagina->bitPresencia==1){

			printf("\nPid: %d, Pag: %d, Marco: %d, Contenido: ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado);
			log_info(dump,"\nPid: %d, Pag: %d, Marco: %d, Contenido: ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado);

			pthread_mutex_lock(&lock_accesoMemoria);
			char* contenido = malloc(config.tamanio_marco);
			memcpy(contenido,memoria+(unaPagina->marcoUtilizado*config.tamanio_marco),config.tamanio_marco);
			pthread_mutex_unlock(&lock_accesoMemoria);
														// C S S
			if(i<cantPaginasDePid(pid)-paginas_stack){ // 0 1 2
				imprimirRegionMemoriaCodigoConsola(contenido, config.tamanio_marco);
				imprimirRegionMemoriaCodigoLogDump(contenido, config.tamanio_marco);
			}else{
				imprimirRegionMemoriaStackConsola(contenido, config.tamanio_marco);
				imprimirRegionMemoriaStackLogDump(contenido, config.tamanio_marco);
			}
		}
//		else{
//			printf("La pagina: %d del pid: %d no esta en memoria \n",unaPagina->nroPagina,pid);
//
//		}
	}
	printf("\n");
	log_info(dump, "------------------------------- \n");
	pthread_mutex_unlock(&lock_accesoTabla);
}

void fRetardo(){
	char* nuevoRetardo = NULL;
	size_t bufsize = 64;
	printf("Ingrese el nuevo valor de Retardo en milisegundos: ");
	getline(&nuevoRetardo,&bufsize,stdin);
	int ret = atoi(nuevoRetardo)*1000;
	retardoMemoria = ret;
}
void dumpEstructuraMemoria(){ //Devuelve todas las tablas de paginas o de un solo pid
	int seleccion=-1;
	int pidDeseado;

	char* selecc = NULL;
	char* selecc2 = NULL;
	size_t bufsize = 64;

	printf("\n");
	printf("0. Devolver todas las tablas |  1. Devolver las paginas de un proceso \n");
	printf("Opcion: ");
	getline(&selecc,&bufsize,stdin);
	seleccion = atoi(selecc);

	switch(seleccion){
		case 0:
			printf("\n");
			devolverTodasLasPaginas(1);
			break;
		case 1:
			printf("De que PID desea listar las paginas? \n");
			printf("Opcion: ");
			getline(&selecc2,&bufsize,stdin);
			pidDeseado = atoi(selecc2);
			printf("\n");
			if(buscarTabla(pidDeseado)){
				devolverPaginasDePid(pidDeseado);
			}else{
				printf("No existe el pid: %d \n",pidDeseado);
			}
			break;
	}
}
void dumpContenidoMemoria(){ //Devuelve toda la memoria o solo la de un pid
	int seleccion=-1;
	int pidDeseado;


	char* selecc = NULL;
	char* selecc2 = NULL;
	size_t bufsize = 64;

	printf("\n");
	printf(" 0. Devolver todas la Memoria|  1. Devolver la memoria de un proceso \n");


	printf("Opcion: ");
	getline(&selecc,&bufsize,stdin);
	seleccion = atoi(selecc);

	switch(seleccion){
		case 0:
			printf("\n");
			devolverTodaLaMemoria();
			break;
		case 1:
			printf("De que PID desea listar la memoria? \n");
			printf("Opcion: ");
			getline(&selecc2,&bufsize,stdin);
			pidDeseado = atoi(selecc2);
			printf("\n");
			if(buscarTabla(pidDeseado)){
				devolverMemoriaDePid(pidDeseado);
			}else{
				printf("No existe el pid: %d \n",pidDeseado);
			}

			break;
	}
}
