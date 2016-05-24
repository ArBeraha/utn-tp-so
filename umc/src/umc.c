/*
 * umc.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "umc.h"

struct timeval newEspera()
{
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}


void cargarCFG() {
	t_config* configUmc;
	configUmc = config_create("umc.cfg");

	config.puerto_swap = config_get_int_value(configUmc, "PUERTO_SWAP");
	config.puerto_umc_nucleo= config_get_int_value(configUmc, "PUERTO_UMC_NUCLEO");
	config.cantidad_marcos = config_get_int_value(configUmc, "CANTIDAD_MARCOS");
	config.tamanio_marco = config_get_int_value(configUmc, "TAMANIO_MARCO");
	config.entradas_tlb = config_get_int_value(configUmc, "ENTRADAS_TLB");
	config.retardo = config_get_int_value(configUmc, "RETARDO");
	config.ip_swap = config_get_string_value(configUmc, "IP_SWAP");
	config.puerto_cpu = config_get_int_value(configUmc, "PUERTO_UMC_CPU");
	config.algoritmo_paginas= config_get_string_value(configUmc, "ALGORITMO_REEMPLAZO");

}

//0. Funciones auxiliares a las funciones Principales


int estaEnTlb(pedidoLectura_t pedido){
	int i;
	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			return 1;
		}
	}
	return 0;
}

int buscarEnTlb(pedidoLectura_t pedido){ //Repito codigo, i know, pero esta soluc no funciona para las dos, porque si se encuentra el pedido en tlb[0] y retornas 'i', "no estaria en tlb" cuando si
	int i;
	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			tlb[i].contadorTiempo = tiempo++;
			return i;
		}
	}
	return 0;
}

int existePidEnListadeTablas(int pid){
	t_list* lista = list_create();
	lista=list_get(listaTablasPaginas, pid);
	if(list_size(lista)==0){
		return 0;
	}else{
		return 1;
	}
}

int existePaginaBuscadaEnTabla(int pag, t_list* tablaPaginaBuscada){
	tablaPagina_t* tabla = malloc(sizeof(tablaPagina_t));
		tabla=list_get(tablaPaginaBuscada, pag);
		if(tabla){
			return 1;
		}else{
			return 0;
		}
}

char* buscarMarco(int marcoBuscado, pedidoLectura_t pedido){ //Necesito esta funcion?
	int pos = marcoBuscado * config.tamanio_marco;
	return &memoria[pos];  //TODO VER!
}

int buscarPrimerMarcoLibre(){
	int i;
	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			return i;
		}
	}
	return -1;
}

int cantidadMarcosLibres(){
	int i;
	int c=0;

	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			c++;
		}
	}
	return c;
}

// FIN 0


// 1. Funciones principales

void reemplazarEntradaConLru(tablaPagina_t* pagina,int pidParam){ //Y la agrega tmb...
	int menorTiempo= tlb[0].contadorTiempo;
	int posicionMenorTiempo = 0;
	int i;
	for(i=0;i<config.entradas_tlb;i++){
		if(tlb[i].contadorTiempo < menorTiempo){
			menorTiempo = tlb[i].contadorTiempo;
			posicionMenorTiempo = i;
		}
	}
	tlb[posicionMenorTiempo].pid=pidParam;
	tlb[posicionMenorTiempo].pagina=pagina->nroPagina;
	tlb[posicionMenorTiempo].marcoUtilizado=pagina->marcoUtilizado;
	tlb[posicionMenorTiempo].contadorTiempo=tiempo++;

}


void agregarATlb(tablaPagina_t* pagina,int pidParam){

	int i;
	for(i=0;i<config.entradas_tlb;i++){
		if(tlb[i].pagina==-1){
			//Se encontro un espacio libre en la tlb, vamos a guardarlo ahi
			tlb[i].pagina = pagina->nroPagina;
			tlb[i].marcoUtilizado = pagina->marcoUtilizado;
			tlb[i].pid = pidParam;
			tlb[i].contadorTiempo = tiempo++;
			return;
		}
	}

	reemplazarEntradaConLru(pagina,pidParam);

}

tablaPagina_t* buscarEnSwap(int marcoBuscado, pedidoLectura_t pedido){
	//TODO
}

int sacarConClock(int pid){

	t_list* tabla = list_get(listaTablasPaginas, pid); //TABLA CON ID A REEMPLAZAR
	int cantidadPaginas = list_size(tabla);
	int posAReemplazar;
	tablaPagina_t* puntero;

	//Primera vuelta, doy segunda oportunidad
	for(posAReemplazar=0;posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get(tabla,posAReemplazar);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=0;posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get(tabla,posAReemplazar);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			return posAReemplazar;
		}
	}

	printf("Algo fallo en CLOCK \n");
}

int sacarConModificado(int pid){ //DESPUES TRATO DE NO REPETIR LOGICA, PRIMERO QUE ANDE!

	t_list* tabla = list_get(listaTablasPaginas, pid); //TABLA CON ID A REEMPLAZAR
	int cantidadPaginas = list_size(tabla);
	int posAReemplazar;
	tablaPagina_t* puntero;

	//Primera vuelta, me fijo si hay alguno (0,0) sin modificar nada
	for(posAReemplazar=0;posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get(tabla,posAReemplazar);
		if(puntero->bitUso==0 && puntero->bitModificacion==0 && puntero->bitPresencia==1){
			return posAReemplazar;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=0;posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get(tabla,posAReemplazar);
		if(puntero->bitUso==0 && puntero->bitModificacion==1 && puntero->bitPresencia==1){
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//PrimerVuelta
	for(posAReemplazar=0;posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get(tabla,posAReemplazar);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=0;posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get(tabla,posAReemplazar);
		if(puntero->bitUso==0 && puntero->bitModificacion==1 && puntero->bitPresencia==1){
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	printf("Algo fallo en CLOCK MODIFICADO \n");
	return -1;
}

void agregarAMemoria(tablaPagina_t* paginaACargar, int pid){

	int posicionPaginaSacada=0;
	if(strcmp(config.algoritmo_paginas,"CLOCK")){
		posicionPaginaSacada=sacarConClock(pid);
	}else {
		if(strcmp(config.algoritmo_paginas,"CLOCK_MODIFICADO")){
			posicionPaginaSacada=sacarConModificado(pid);
		}
		else{
			printf("Error sintaxis algoritmo: CLOCK o CLOCK_MODIFICADO");
		}

	}

	t_list* tablaPaginaAReemplazar = list_get(listaTablasPaginas, pid);
	tablaPagina_t* paginaAReemplazar = list_get(tablaPaginaAReemplazar, posicionPaginaSacada);

	paginaAReemplazar->nroPagina = paginaACargar->nroPagina;
	paginaAReemplazar->marcoUtilizado = paginaACargar->marcoUtilizado;
	paginaAReemplazar->bitPresencia = 1;
	paginaAReemplazar->bitModificacion = 0;
	paginaAReemplazar->bitUso=0;
}

char* devolverPedidoPagina(pedidoLectura_t pedido){

	//SI ESTA EN TLB DEVUELVO

	if(estaEnTlb(pedido) && config.entradas_tlb){
		log_info(activeLogger,"Se encontro en la Tlb el pid: %d, pagina: %d PARA LECTURA \n",pedido.pid,pedido.paginaRequerida);
		int pos = buscarEnTlb(pedido);

		char* contenido = malloc(pedido.cantBytes + 1);

		printf("Accediendo a memoria... \n");
		usleep(retardoMemoria);

		memcpy(contenido,memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes); // FALLANDO
		contenido[pedido.cantBytes]='\0';

		printf("marco tlb: %d \n", tlb[pos].marcoUtilizado);
		return contenido; //Provisorio, tiene que ser un SEND

		//send_w(cliente, contenido, 4);

	//HASTA ACA OK!

	}
	//SINO, ME FIJO QUE SEA VALIDA LA PETICION
	else{
		log_info(activeLogger,"No se encontro en la Tlb el pid: %d, pagina: %d. Se buscara en la Lista de tablas de paginas",pedido.pid,pedido.paginaRequerida);
		if(existePidEnListadeTablas(pedido.pid)){ //Si existe la tabla de paginas dentro de la lista
			t_list* tablaPaginaBuscada = list_get(listaTablasPaginas, pedido.pid);
			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				tablaPagina_t* paginaBuscada = list_get(tablaPaginaBuscada, pedido.paginaRequerida);

	//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB
				if(paginaBuscada->bitPresencia){

					log_info(activeLogger,"Se encontro la pagina y esta en memoria! Devolviendo pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

					printf("Accediendo a memoria... \n");
					usleep(retardoMemoria);

					char* contenido = malloc(pedido.cantBytes + 1);
					memcpy(contenido,memoria+paginaBuscada->marcoUtilizado * config.tamanio_marco+pedido.offset,pedido.cantBytes);
					contenido[pedido.cantBytes]='\0';

					printf("marco tlb: %d \n", paginaBuscada->marcoUtilizado);


					agregarATlb(paginaBuscada,pedido.pid);

					//send_w(cliente, devolucion, 4);
					return contenido; //Provisorio, tiene que ser un SEND
		//HASTA ACA ANDA FENOMENO!
				}
	// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y VUELVO A LLAMAR A FUNCION
				else{
					tablaPagina_t* devolucion = buscarEnSwap(paginaBuscada->marcoUtilizado,pedido);
					agregarAMemoria(devolucion,pedido.pid);
					devolucion->bitPresencia=1;
					devolucion->bitModificacion=0;
					agregarATlb(devolucion,pedido.pid);
					devolverPedidoPagina(pedido);
				}
			}
	// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
				send_w(cliente, intToChar(HeaderNoExistePagina), 4);
			}
		}
	// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		else{
			send_w(cliente,intToChar(HeaderNoExisteTablaDePag), 4);
		}
	}
}

int inicializarPrograma(int idPrograma, char* contenido){

	send_w(swapServer,intToChar(HeaderConsultaEspacioSwap),4);
	send_w(swapServer,intToChar(strlen(contenido)),4);
	char* tieneEspacio;
	read(swapServer , &tieneEspacio, 4);

	if(atoi(tieneEspacio)){
		send_w(swapServer,intToChar(idPrograma),4);
		send_w(swapServer,contenido,strlen(contenido));
		return 1;
	}
	else{
		return 0;
	}
}

char* almacenarBytesEnUnaPagina(pedidoLectura_t pedido, int size, char* buffer){  //TODO Falta lo de swap

	if(estaEnTlb(pedido) && config.entradas_tlb){

		log_info(activeLogger,"Se encontro en la Tlb el pid: %d, pagina: %d PARA ESCRITURA \n",pedido.pid,pedido.paginaRequerida);

		int pos = buscarEnTlb(pedido);

		printf("Posicion encontrada en la TLB: %d \n \n",pos);

		printf("Accediendo a memoria... \n ");
		usleep(retardoMemoria);

		memcpy(memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, buffer, strlen(buffer)); //size??? PARA QUE??

		printf("marco tlb: %d \n", tlb[pos].marcoUtilizado);

		printf("Lo que acabo de almacenar: %s .\n \n ",memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset);
		printf("Ahora llamo a la funcion devolverPedidoPagina (conecto las dos func) \n \n");
		return (devolverPedidoPagina(pedido)); //Provisorio para testear

	}
	else{
		log_info(activeLogger,"No se encontro en la Tlb el pid: %d, pagina: %d. Se buscara en la Lista de tablas de paginas",pedido.pid,pedido.paginaRequerida);
		printf("PID: %d \n",pedido.pid);
		if(existePidEnListadeTablas(pedido.pid)){ //Si existe la tabla de paginas dentro de la lista
		t_list* tablaPaginaBuscada = list_get(listaTablasPaginas, pedido.pid);

			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				tablaPagina_t* paginaBuscada = list_get(tablaPaginaBuscada, pedido.paginaRequerida);

	//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB
				if(paginaBuscada->bitPresencia){

					log_info(activeLogger,"Se encontro la pagina y esta en memoria! Escribiendo pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

					printf("Accediendo a memoria... \n ");
					usleep(retardoMemoria);

					paginaBuscada->bitModificacion = 1;  //NEW
					memcpy(memoria+paginaBuscada->marcoUtilizado*config.tamanio_marco+pedido.offset, buffer, strlen(buffer)); //size??? PARA QUE??

					printf("Marco de la pagina: %d \n", paginaBuscada->marcoUtilizado);

					printf("Lo que acabo de almacenar: %s .\n \n ",memoria+paginaBuscada->marcoUtilizado*config.tamanio_marco+pedido.offset);
					printf("Ahora llamo a la funcion devolverPedidoPagina (conecto las dos func) \n \n");

					agregarATlb(paginaBuscada,pedido.pid);

					//send_w(cliente, devolucion, 4);
					return (devolverPedidoPagina(pedido)); //Provisorio para testear

				}
	// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y TODO LA CARGO EN MEMORIA Y TLB Y RECIEN AHI LA DEVUELVOl, SI NO HAY PAGINAS DISPONIBLES: ALGORITMO DE SUSTITUCION DE PAGINAS
				else{
					char* contenidoEnSwap = buscarEnSwap(paginaBuscada->marcoUtilizado,pedido);
					agregarAMemoria(paginaBuscada,pedido.pid);
					almacenarBytesEnUnaPagina(pedido,size,buffer);
				}
			}// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
					send_w(cliente, intToChar(HeaderNoExistePagina), 4);
			}
			// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		}
		else{
				send_w(cliente,intToChar(HeaderNoExisteTablaDePag), 4);
		}
	}
	return "Error ifs";
}

void sacarMarcosOcupados(int idPrograma){
	t_list* auxiliar = list_create();
	auxiliar = list_get(listaTablasPaginas,idPrograma);

	tablaPagina_t* tabla = malloc(sizeof(tablaPagina_t));
	int i;
	int size = list_size(auxiliar);
	for(i=0;i<size;i++){
		tabla = list_get(auxiliar,i);
		vectorMarcosOcupados[tabla->marcoUtilizado] = 0;
	}
}

void finalizarPrograma(int idPrograma){
	sacarMarcosOcupados(idPrograma);
	list_destroy(list_get(listaTablasPaginas,idPrograma));
}

//FIN 1


//2. Funciones que se mandan por consola

void devolverTodasLasPaginas(){  //OK
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		t_list* unaTabla = malloc(sizeof(t_list));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size(unaTabla);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,j);

			printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",i,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
			log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",i,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
		}
	}
}

void devolverPaginasDePid(int pid){ //OK
	t_list* unaTabla = malloc(sizeof(t_list));

	if(existePidEnListadeTablas(pid)){

			unaTabla = list_get(listaTablasPaginas,pid);
			int cantidadPaginasDeTabla = list_size(unaTabla);
			int i;

			for(i=0;i<cantidadPaginasDeTabla;i++){
				tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
				unaPagina = list_get(unaTabla,i);
				printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
				log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
			}
	}else{
		printf("Ese pid no existe o no esta en uso! \n");
	}
}

void imprimirRegionMemoria(char* region, int size){
	int i;
	for(i=0;i<size;i++){
			putchar(region[i]);
	}
}

void devolverTodaLaMemoria(){

	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		t_list* unaTabla = malloc(sizeof(t_list));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size(unaTabla);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,j);
			//Hago un solo print f de las caracteristicas
			printf("Accediendo a memoria... \n ");
			usleep(retardoMemoria);

			printf("Pid: %d, Pag: %d, Marco: %d, Contenido: ",i, unaPagina->nroPagina,unaPagina->marcoUtilizado);

			char* contenido = malloc(config.tamanio_marco+1);
			memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
			contenido[config.tamanio_marco]='\0';

			imprimirRegionMemoria(contenido,config.tamanio_marco);

			log_info(dump,"Pid: %d, Pag: %d, Marco: %d, Contenido: %s ",i, unaPagina->nroPagina,unaPagina->marcoUtilizado,contenido);

			printf("\n");
		}
	}
	printf("\n");
}

void devolverMemoriaDePid(int pid){ //OK
	t_list* unaTabla = malloc(sizeof(t_list));
	int tamanioLista = list_size(listaTablasPaginas);

	if(pid<=tamanioLista){
		unaTabla = list_get(listaTablasPaginas,pid);
		int cantidadPaginasDeTabla = list_size(unaTabla);
		int i;

		for(i=0;i<cantidadPaginasDeTabla;i++){
			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,i);

			if(unaPagina->bitPresencia!=1){

				printf("Accediendo a memoria... \n ");
				usleep(retardoMemoria);

				printf("Pid: %d, Pag: %d, Marco: %d, Contenido: ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado);

				char* contenido = malloc(config.tamanio_marco+1);
				memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
				contenido[config.tamanio_marco]='\0';

				imprimirRegionMemoria(contenido,config.tamanio_marco);
				log_info(dump, "Pid: %d, Pag: %d, Marco: %d, Contenido: %s ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,contenido);

				printf("\n");
			}
			else{
				printf("La pagina: %d del pid: %d no esta en memoria \n",unaPagina->nroPagina,pid);
			}
		}
		printf("\n");
	}
	else{
		printf("El pid supera la cantidad de tablas \n");
		log_info(dump, "El pid supera la cantidad de tablas ");
	}
}

void fRetardo(){
	char* nuevoRetardo = NULL;
	size_t bufsize = 64;
	printf("Ingrese el nuevo valor de Retardo en milisegundos: ");
	getline(&nuevoRetardo,&bufsize,stdin);
	int ret = atoi(nuevoRetardo);
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
			devolverTodasLasPaginas();
			break;
		case 1:
			printf("De que PID desea listar las paginas? \n");
			printf("Opcion: ");
			getline(&selecc2,&bufsize,stdin);
			pidDeseado = atoi(selecc2);
			printf("\n");
			devolverPaginasDePid(pidDeseado);
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
			devolverMemoriaDePid(pidDeseado);
			break;
	}
}
void flushTlb(){
	inicializarTlb();
}
void flushMemory(){ //Pone a todas las paginas bit de modificacion en 1
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;
	for(i=0;i<cantidadTablas;i++){

		t_list* unaTabla = list_get(listaTablasPaginas,i);
		int cantidadPaginasDeTabla = list_size(unaTabla);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = list_get(unaTabla,j);
			unaPagina->bitModificacion=1;
		}
	}
}

void recibirComandos(){  //ANDA OK
	int funcion;
	do {
		char* selecc = NULL;
		size_t bufsize = 64;

		printf(" \n \n");
		printf("Funciones: 0.salir / 1.retardo / 2.dumpEstructuraMemoria / 3.dumpContenidoMemoria / 4.flushTlb / 5.flushMemory \n");
		printf("Funcion: ");

		getline(&selecc,&bufsize,stdin);
		funcion = atoi(selecc);

		switch(funcion){
			case 1: fRetardo(); break;
			case 2: dumpEstructuraMemoria();break;
			case 3: dumpContenidoMemoria();break;
			case 4: flushTlb();break;
			case 5: flushMemory();break;
			default: break;
		}
	}
	while(funcion!=0);
}
// FIN 2

// 3. Inicializar estructura de UMC
void inicializarTlb(){
	tiempo=0;
	int i;
	for(i = 0; i<config.entradas_tlb; i++){
		tlb[i].pid=-1;
		tlb[i].pagina=-1;
		tlb[i].marcoUtilizado=-1;
		tlb[i].contadorTiempo=-1;
	}
}

void crearMemoriaYTlbYTablaPaginas(){

	//Creo memoria y la relleno
	tamanioMemoria = config.cantidad_marcos * config.tamanio_marco;
	memoria = malloc(tamanioMemoria);
	memset(memoria,'\0',tamanioMemoria);
	log_info(activeLogger,"Creada la memoria.\n");

	//Relleno TLB
	tlb = malloc(config.entradas_tlb * sizeof(tlb_t));
	inicializarTlb();
	log_info(activeLogger,"Creada la TLB y rellenada con ceros (0).\n");

	//Creo vector de marcos ocupados y lo relleno
	vectorMarcosOcupados = malloc(sizeof(int) * config.cantidad_marcos);
	log_info(activeLogger,"Creado el vector de marcos ocupados \n");

	memset(vectorMarcosOcupados,0,sizeof(int) * config.cantidad_marcos);

	ultimoByteOcupado = malloc(config.cantidad_marcos * sizeof(int));
	memset(ultimoByteOcupado,0,sizeof(int) * config.cantidad_marcos);

	vectorClientes = malloc(MAXCLIENTS * sizeof(int));
	memset(vectorClientes,-1, MAXCLIENTS * sizeof(int));

	retardoMemoria = config.retardo;

//	vectorHilosCpu = malloc(sizeof(pthread_t) * MAXCLIENTS);

	pilaAccesosTlb = stack_create();

}
// FIN 3

// 4. Procesar headers

int primerNumeroPaginaLibre(int pid){
	t_list* auxiliar = list_create();
	auxiliar= list_get(listaTablasPaginas,pid);
	return list_size(auxiliar);
}

void sacarPaginasConClock(int cantidadPaginas){ //TODO

}

int reservarPagina(int cantPaginasPedidas, int pid){ // OK

	if(cantidadMarcosLibres()>=cantPaginasPedidas){ //Si alcanzan los marcos libres...

		int i;
		for(i=0;i<cantPaginasPedidas;i++){

			tablaPagina_t *nuevaPag = malloc(sizeof(tablaPagina_t));

			int unMarcoNuevo = buscarPrimerMarcoLibre();
			vectorMarcosOcupados[unMarcoNuevo]=1; //Lo marco como ocupado
			printf("Marco seleccionado numero: %d (deberia ser 4, 5 y 6)\n", unMarcoNuevo);

			int posicion;
			if(existePidEnListadeTablas(pid)){
				posicion = primerNumeroPaginaLibre(pid);
			}else{
				posicion = i;
			}

			nuevaPag->nroPagina = posicion;
			nuevaPag->marcoUtilizado = unMarcoNuevo;
			nuevaPag->bitPresencia=1;
			nuevaPag->bitModificacion=0;
			nuevaPag->bitUso=1;

			t_list* tablaPag = list_get(listaTablasPaginas,pid);
			list_add_in_index(tablaPag,posicion,nuevaPag);
		}
		return 1;
	}
	else {
//		sacarPaginasConClock(cantPaginasPedidas); MEPA QUE NO VA!
		return 0;
	}
}



void esperar_header(int cliente) {
	log_debug(bgLogger, "Esperando header del cliente: %d., cliente");
	char* header;
	while (1) {
		header = recv_waitall_ws(clientes[cliente].socket, 1);
		procesarHeader(cliente,header);
		free(header);
	}
}

char* getScript(int clienteNucleo) {
	log_debug(bgLogger, "Recibiendo archivo de nucleo %d...");
	char scriptSize;
	char* script;
	int size;
	read(clientes[clienteNucleo].socket, &scriptSize, sizeof(int));
	size = char4ToInt(&scriptSize);
	printf("%d",size);
	log_debug(bgLogger, "Nucleo envió un archivo de tamaño: %d", size);
	printf("Size:%d\n", size);
	script = malloc(sizeof(char) * size);
	read(clientes[clienteNucleo].socket, script, size);
	log_info(activeLogger, "Script de nucleo %d recibido:\n%s", clienteNucleo, script);
	clientes[clienteNucleo].atentido=false; //En true se bloquean, incluso si mando muchos de una consola usando un FOR para mandar el comando (leer wikia)
	return script;
}

void procesarHeader(int cliente, char *header){
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger,"Llego un mensaje con header %d\n",charToInt(header));
	clientes[cliente].atentido=true;

	switch(charToInt(header)) {

	case HeaderError:
		log_error(activeLogger,"Header de Error\n");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger,"Llego un handshake\n");
		payload_size=1;
		payload = malloc(payload_size);
		read(clientes[cliente].socket , payload, payload_size);
		log_debug(bgLogger,"Llego un mensaje con payload %d\n",charToInt(payload));
		if (charToInt(payload)==SOYCPU){
			log_debug(bgLogger,"Es un cliente apropiado! Respondiendo handshake\n");
			clientes[cliente].identidad = charToInt(payload);
			send(clientes[cliente].socket, intToChar(SOYUMC), 1, 0);
			pthread_create(&(vectorHilosCpu[cliente]),NULL,(void*)esperar_header,(void*)cliente);

		}else if(charToInt(payload)==SOYNUCLEO){
			log_debug(bgLogger,"Es un cliente apropiado! Respondiendo handshake\n");
			clientes[cliente].identidad = charToInt(payload);
			send(clientes[cliente].socket, intToChar(SOYUMC), 1, 0);
		}
		else {
			log_error(activeLogger,"No es un cliente apropiado! rechazada la conexion\n");
			log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
			quitarCliente(cliente);
		}
		free(payload);
		clientes[cliente].atentido=false;
		break;

		case HeaderReservarEspacio:

			pedidoPaginaPid = recv_waitall_ws(cliente, sizeof(int));
			pedidoPaginaTamanioContenido = recv_waitall_ws(cliente, sizeof(int));
			//ES NECESARIO TENER EL PID DEL PROCESO Q NUCLEO QUIERE GUARDAR EN MEMORIA? SI: RECIBIR INT  NO: RECIBIR NADA
			log_info(activeLogger,"Nucleo me pidio memoria");

			int cantPaginasPedidas = ((float)charToInt(pedidoPaginaTamanioContenido) + config.tamanio_marco - 1) / config.tamanio_marco; //A+B-1 / B
			int pid = charToInt(pedidoPaginaPid);

			//TODO Primero: if swap tiene espacio..

			if(reservarPagina(cantPaginasPedidas,pid)){
				send_w(cliente, headerToMSG(HeaderTeReservePagina), 1);
			}
			else{
				send_w(cliente, headerToMSG(HeaderErrorNoHayPaginas), 1);
			};
			break;

		case HeaderTamanioPagina:
			break;

		case HeaderPedirValorVariable:
			log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
			pedidoLectura_t pedido;
			pedido.pid = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
			pedido.paginaRequerida = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
			pedido.offset = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
			pedido.cantBytes = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
			send_w(cliente,devolverPedidoPagina(pedido),sizeof(int));
			break;

//		case HeaderInicializarPrograma:
//			char* idPrograma = recv_waitall_ws(cliente,4);
//			char* contenido = getScript(cliente);

//			inicializarPrograma(idPrograma,contenido);

		case HeaderGrabarPagina:
			log_info(activeLogger,"Se recibio pedido de grabar una pagina, por CPU");
			break;

		case HeaderLiberarRecursosPagina:
			log_info(activeLogger,"Se recibio pedido de liberar una pagina, por CPU");
			break;

		default:
			log_error(activeLogger,"Llego cualquier cosa.");
			log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
			log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
			quitarCliente(cliente);
			break;
	}
}

// FIN 4


void finalizar() {
	destruirLogs();
	log_destroy(dump);
	list_destroy(listaTablasPaginas);
	free(memoria);
}


int main(void) {

	cargarCFG();

	crearLogs("Umc","Umc");

	dump = log_create("dump","UMC",false,LOG_LEVEL_INFO);

	log_info(activeLogger,"Soy umc de process ID %d.\n", getpid());

	listaTablasPaginas = list_create();

	int k;
	for(k=0;k<config.cantidad_marcos;k++){  //COMO MAXIMO ES LA CANTIDAD DE MARCOS, considerando q como minimo una tabla tiene 1 pag
		t_list* tablaPaginas = list_create();
		list_add(listaTablasPaginas,tablaPaginas);
	}

	crearMemoriaYTlbYTablaPaginas();

	test();

//	pthread_create(&hiloRecibirComandos,NULL,(void*)recibirComandos,NULL);

//	servidorCPUyNucleoExtendido();

//	conexionASwap();

	finalizar();

	return 0;
}






























void test(){

	vectorMarcosOcupados[0]=1;
	vectorMarcosOcupados[1]=1;
	vectorMarcosOcupados[2]=1;

	int marcoNuevo = buscarPrimerMarcoLibre();
	vectorMarcosOcupados[marcoNuevo]=1; //Lo marco como ocupado

	printf("El primer marco libre deberia ser el 3 y es: %d \n", marcoNuevo);
	printf("Y ahora su contenido deberia ser 1: %d \n\n", vectorMarcosOcupados[marcoNuevo]);

	printf("Contenido vector en pos 0: %d \n", vectorMarcosOcupados[0]);
	printf("Contenido vector en pos 1: %d \n", vectorMarcosOcupados[1]);
	printf("Contenido vector en pos 2: %d \n", vectorMarcosOcupados[2]);
	printf("Contenido vector en pos 3: %d \n", vectorMarcosOcupados[3]);
	printf("Contenido vector en pos 4: %d \n", vectorMarcosOcupados[4]);
	printf("Contenido vector en pos 5: %d \n", vectorMarcosOcupados[5]);
	printf("Contenido vector en pos 6: %d \n", vectorMarcosOcupados[6]);
	printf("Contenido vector en pos 7: %d \n", vectorMarcosOcupados[7]);
	printf("Contenido vector en pos 8: %d \n", vectorMarcosOcupados[8]);
	printf("Contenido vector en pos 9: %d \n", vectorMarcosOcupados[9]);

	printf("Pasamos al test de memoria \n \n");

	int test = reservarPagina(3,5);

	if(test){

		tabla5 = list_get(listaTablasPaginas, 5);

		tablaPagina_t* pagina0Tabla5 = list_get(tabla5,0);
		tablaPagina_t* pagina1Tabla5 = list_get(tabla5,1);
		tablaPagina_t* pagina2Tabla5 = list_get(tabla5,2);

		printf("Agarramos la tabla de paginas en las posicion 5. \n");
		printf("Y deberia tener 3 paginas dentro, coincide con cant: %d \n", list_size(tabla5));
		printf("En la posicion 0 estaria la pagina 0 con marco 4, coincide con: pagina:%d, marco: %d \n", pagina0Tabla5->nroPagina, pagina0Tabla5->marcoUtilizado);
		printf("En la posicion 0 estaria la pagina 1 con marco 5, coincide con: pagina:%d, marco: %d \n", pagina1Tabla5->nroPagina, pagina1Tabla5->marcoUtilizado);
		printf("En la posicion 0 estaria la pagina 2 con marco 6, coincide con: pagina:%d, marco: %d \n", pagina2Tabla5->nroPagina, pagina2Tabla5->marcoUtilizado);
	}
	else{
		printf("No hay paginas disponibles");
	}

	memoria[5*config.tamanio_marco]='a';
	memoria[5*config.tamanio_marco+1]='b';
	memoria[5*config.tamanio_marco+2]='c';
	memoria[5*config.tamanio_marco+3]='d';
	memoria[5*config.tamanio_marco+11]='d';

	memoria[6*config.tamanio_marco]='a';
	memoria[6*config.tamanio_marco+1]='b';
	memoria[6*config.tamanio_marco+2]='c';

	devolverTodaLaMemoria();

	printf("---- De solo pid 5 --- \n");
	devolverMemoriaDePid(5);

	printf(" -------------------------------------------  \n \n");
	printf("Haciendo un pedido de pagina \n");

	pedidoLectura_t pedido;
	pedido.pid = 5;
	pedido.paginaRequerida = 1;
	pedido.offset = 2;
	pedido.cantBytes = 2;

	printf("Simulamos que esta en TLB \n");
	tlb[3].pid=5;
	tlb[3].pagina=1;
	tlb[3].marcoUtilizado= 5;

	char* respuesta = devolverPedidoPagina(pedido);
	printf("Respuesta1 deberia ser 'cd' y es: %s  \n",respuesta); //Original: abcd Empieza de la posicion 2 y lee 2 bytes

	printf("--***---\n");

	printf("Reinicio la tlb y no deberia encontrar, paso a buscarla en lista tablas \n");
	inicializarTlb();
	char* respuesta2 = devolverPedidoPagina(pedido);
	printf("Respuesta2 deberia ser 'cd' y es: %s  \n",respuesta2);
	printf("Y se deberia haber agregado a tlb.. \n");
	printf("Tlb deberia ser (5,1,5) y es : Pid: %d, Pagina: %d, Marco: %d \n", tlb[0].pid, tlb[0].pagina, tlb[0].marcoUtilizado);
	printf("--***---\n");

	printf(" -------------------------------------------  \n \n");

	printf("Pedido escritura \n");
	pedidoLectura_t pedido2;
	pedido2.pid=2;
	pedido2.paginaRequerida=0;
	pedido2.offset = 0;
	pedido2.cantBytes = 4;

	reservarPagina(3,2);
	tlb[2].pagina=0;
	tlb[2].pid = 2;
	tlb[2].marcoUtilizado=7;

	pedidoLectura_t pedido3;
	pedido3.pid=2;
	pedido3.paginaRequerida = 1;
	pedido3.offset = 2;
	pedido3.cantBytes = 4;

	tlb[5].pagina=1;
	tlb[5].pid = 2;
	tlb[5].marcoUtilizado=7;


	printf("\n \n \n \n");
	printf("Primer pedido de escritura: \n");
	printf("Devolucion1: %s \n", almacenarBytesEnUnaPagina(pedido2,4,"3"));

	printf("\n \n \n \n");
	printf("Segundo pedido de escritura: \n");
	printf("Devolucion2: %s \n", almacenarBytesEnUnaPagina(pedido3,4,"311"));

//	printf("Devolucion: %s \n", almacenarBytesEnUnaPagina(pedido2,4,'abc'));


	printf("Ahora devuelvo toda la memoria para ver que no haya nada raro.. \n");

	devolverTodaLaMemoria();

	printf("Limpio la tlb y vuelvo a pedir, deberia encontrarla en tabla paginas.. \n");
	inicializarTlb();
	pedidoLectura_t pedido4;
	pedido4.pid=2;
	pedido4.paginaRequerida = 1;
	pedido4.offset = 0;
	pedido4.cantBytes = 4;
	printf("----La tlb[0] estaba vacia: pid:%d, pag:%d, marco:%d \n", tlb[0].pid,tlb[0].pagina,tlb[0].marcoUtilizado);
	printf("Devolucion3: %s \n", almacenarBytesEnUnaPagina(pedido4,4,"44"));
	printf("----La tlb[0] ahora ocupada: pid:%d, pag:%d, marco:%d \n", tlb[0].pid,tlb[0].pagina,tlb[0].marcoUtilizado);


	printf("Vuelvo a devolver la memoria entera \n");
	devolverTodaLaMemoria();

	printf(" -------------------------------------------  \n \n");

	pedidoLectura_t pedido5;
	pedido5.pid=7;
	pedido5.paginaRequerida = 1;
	pedido5.offset = 4;
	pedido5.cantBytes = 4;

	tlb[1].pid = 7;
	tlb[1].pagina = 1;
	tlb[1].marcoUtilizado = 11;

	int pudo = reservarPagina(3,7);

//	printf("Asignando memoria contigua \n \n");
//
//	printf("Devolucion 4: %s \n", almacenarBytesEnUnaPaginaContiguo(pedido5,4,"27"));

	devolverTodaLaMemoria();

	printf(" -------------------------------------------  \n \n");
	printf(" -------------------------------------------  \n \n");

	printf("Test inicializar programa \n \n ");

//	inicializarPrograma(4,"123456789123456789123456789123456789123456789123456789123456789"); //36 bytes

	devolverTodaLaMemoria();
	devolverTodasLasPaginas();

	printf("\n \n");
	printf("Y si finalizo, la tabla de pags y de memoria: \n \n");

//	finalizarPrograma(4);
	devolverTodaLaMemoria();
	devolverTodasLasPaginas();

	printf(" -------------------------------------------  \n \n");
	printf(" -------------------------------------------  \n \n");


	flushTlb();
	tlb[0].pid=1;
	tlb[0].pagina=4;
	tlb[0].marcoUtilizado=3;
	tlb[0].contadorTiempo=0;
	tlb[1].pid=2;
	tlb[1].pagina=3;
	tlb[1].marcoUtilizado=5;
	tlb[1].contadorTiempo=0;
	tlb[2].pid=3;
	tlb[2].pagina=2;
	tlb[3].marcoUtilizado=8;
	tlb[4].contadorTiempo=0;

	pedidoLectura_t pedido7;
	pedido7.pid=1;
	pedido7.paginaRequerida = 4;
	pedido7.offset = 4;
	pedido7.cantBytes = 4;
	pedidoLectura_t pedido8;
	pedido8.pid=2;
	pedido8.paginaRequerida = 3;
	pedido8.offset = 4;
	pedido8.cantBytes = 4;
	pedidoLectura_t pedido9;
	pedido9.pid=3;
	pedido9.paginaRequerida = 2;
	pedido9.offset = 4;
	pedido9.cantBytes = 4;

	buscarEnTlb(pedido7);
	buscarEnTlb(pedido8);
	buscarEnTlb(pedido9);
	buscarEnTlb(pedido7);

	tablaPagina_t pagina;
	pagina.nroPagina=1;
	pagina.marcoUtilizado=11;
	tablaPagina_t* paginaAgregar = &pagina;
	agregarATlb(paginaAgregar,4);
	printf("En la posicion 1 deberia estar pid 4 \n");
	printf("Pid: %d con contador: %d \n",tlb[0].pid,tlb[0].contadorTiempo);
	printf("Pid: %d con contador: %d \n",tlb[1].pid,tlb[1].contadorTiempo);
	printf("Pid: %d con contador: %d \n",tlb[2].pid,tlb[2].contadorTiempo);
	printf("Ahora vuelve pedido con pid 2, que habia sido sacado");
	tablaPagina_t pagina2;
	pagina.nroPagina=3;
	pagina.marcoUtilizado=12;
	tablaPagina_t* paginaAgregar2 = &pagina2;
	agregarATlb(paginaAgregar2,2);
	printf("En ultima posicion deberia estar pid 2 \n");
	printf("Pid: %d con contador: %d \n",tlb[0].pid,tlb[0].contadorTiempo);
	printf("Pid: %d con contador: %d \n",tlb[1].pid,tlb[1].contadorTiempo);
	printf("Pid: %d con contador: %d \n",tlb[2].pid,tlb[2].contadorTiempo);


	printf(" -------------------------------------------  \n \n");
	printf(" -------------------------------------------  \n \n");

//	recibirComandos();
	t_list* tabla = list_get(listaTablasPaginas,2);
	tablaPagina_t* pag2 = list_get(tabla,2);
	pag2->bitUso=0;

	devolverPaginasDePid(2);

	printf("Pagina a sacar con clock modificado de pid 2: %d \n", sacarConModificado(2));
	printf("Pagina a sacar con clock de pid 2: %d \n", sacarConClock(2));

}



// 5.Server de los cpu y de nucleo

void servidorCPUyNucleoExtendido(){

	struct timeval espera = newEspera();
	int i;
	char header[1];

	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
				&tamanioDireccionCPU, &activadoCPU);
	configurarServidorExtendido(&socketNucleo, &direccionNucleo, config.puerto_umc_nucleo,
					&tamanioDireccionNucleo, &activadoNucleo);

	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones ...");

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);
		FD_SET(socketNucleo, &socketsParaLectura);

		mayorDescriptor = (socketNucleo>socketCPU) ? socketNucleo : socketCPU;
		incorporarClientes();

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, &espera);

		if (tieneLectura(socketNucleo))
			procesarNuevasConexionesExtendido(&socketNucleo);

		if (tieneLectura(socketCPU))
			procesarNuevasConexionesExtendido(&socketCPU);

		for (i = 0; i < getMaxClients(); i++) {
			if (tieneLectura(clientes[i].socket)) {
				if (read(clientes[i].socket, header, 1) == 0) {
					log_error(activeLogger,
					"Un cliente se desconectó.");
					quitarCliente(i);
				} else
					procesarHeader(i, header);
				}
			}

			//Hacer algo?
	}
}


void servidorCPUyNucleo(){

	int mayorDescriptor, i;
	struct timeval espera = newEspera(); 		// Periodo maximo de espera del select
	char header[1];

	crearLogs("Umc","Umc");

	configurarServidor(config.puerto_umc_nucleo);
	inicializarClientes();
	log_info(activeLogger,"Esperando conexiones ...");

	while(1){
		mayorDescriptor = incorporarSockets();
		select( mayorDescriptor + 1 , &socketsParaLectura , NULL , NULL , &espera);

		if (tieneLectura(socketNuevasConexiones))
			procesarNuevasConexiones();
	}
}

int getHandshake()
{
	char* handshake = recv_nowait_ws(swapServer,1);
	return charToInt(handshake);
}

// FIN 5


// 6. Conexion a Swap
void handshakearASwap(){
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYUMC);
	send_w(swapServer, hand, 2);

	log_debug(bgLogger,"Umc handshakeo.");
	if(getHandshake()!=SOYSWAP)
	{
		perror("Se esperaba que la umc se conecte con el swap.");
	}
	else
		log_debug(bgLogger,"Umc recibio handshake de Swap.");
}

void conectarASwap(){
	direccion = crearDireccionParaCliente(config.puerto_swap,config.ip_swap);  //CAMBIAR ESTO DE IP
	swapServer = socket_w();
	connect_w(swapServer, &direccion);

	handshakearASwap();
}

void realizarConexionASwap()
{
	conectarASwap();
	log_info(activeLogger,"Conexion a swap correcta :).");
	handshakearASwap();
	log_info(activeLogger,"Handshake finalizado exitosamente.");
	log_debug(bgLogger,"Esperando algo para imprimir en pantalla.");
}

void escucharPedidosDeSwap(){
	char* header;
	while(true){
		if (swapServer!=0){ // Solo si esta conextado
//			header = recv_waitall_ws(swapServer,sizeof(char)); //ESTO NO ME PERMITE CHEQUEAR SI SE DESCONECTO!!
			header = malloc(1);
			int bytesRecibidos = recv(swapServer, header, 1, MSG_WAITALL);
			if (bytesRecibidos <= 0){
				 printf("SWAP se desconecto\n");
				 close(swapServer);
				 swapServer=0;
				 return;
			}
			else
				procesarHeader(swapServer,header);

			free(header);
		}
	}
}

void conexionASwap(){ //Creada para unir las dos funciones y crear un hilo
	realizarConexionASwap();
	escucharPedidosDeSwap();

}

// FIN 6
