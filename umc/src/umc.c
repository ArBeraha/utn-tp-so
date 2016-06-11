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
	config.retardo = config_get_int_value(configUmc, "RETARDO") * 1000;
	config.ip_swap = config_get_string_value(configUmc, "IP_SWAP");
	config.puerto_cpu = config_get_int_value(configUmc, "PUERTO_UMC_CPU");
	config.algoritmo_paginas= config_get_string_value(configUmc, "ALGORITMO_REEMPLAZO");
	config.marcos_x_proceso = config_get_int_value(configUmc, "MARCOS_X_PROCESO");

}

//0. Funciones auxiliares a las funciones Principales

int buscarUltimaPosSacada(int pidParam){
	int i;
	int size = list_size(listaUltimaPosicionSacada);
	for(i=0;i<size;i++){
		ultimaSacada_t* entrada = list_get(listaUltimaPosicionSacada,i);
		if(entrada->pid == pidParam){
			return entrada->posicion;
		}
	}
	return -1;
}

void cambiarUltimaPosicion(int pidParam, int ultima){
	int i;
		int size = list_size(listaUltimaPosicionSacada);
		for(i=0;i<size;i++){
			ultimaSacada_t* entrada = list_get(listaUltimaPosicionSacada,i);
			if(entrada->pid == pidParam){
				entrada->posicion=ultima;
			}
		}
}


int estaEnTlb(pedidoLectura_t pedido){

	pthread_mutex_lock(&lock_accesoTlb);
	int i;

	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			pthread_mutex_unlock(&lock_accesoTlb);
			return 1;
		}
	}
	pthread_mutex_unlock(&lock_accesoTlb);
	return 0;
}

int buscarEnTlb(pedidoLectura_t pedido){ //Repito codigo, i know, pero esta soluc no funciona para las dos, porque si se encuentra el pedido en tlb[0] y retornas 'i', "no estaria en tlb" cuando si
	pthread_mutex_lock(&lock_accesoTlb);
	int i;
	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			tlb[i].contadorTiempo = tiempo++;
			pthread_mutex_unlock(&lock_accesoTlb);
			return i;
		}
	}
	pthread_mutex_unlock(&lock_accesoTlb);
	return 0;
}

int buscarPosicionTabla(int pidBusca){
	int size = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<size;i++){
		tabla_t* tabla;
		tabla = list_get(listaTablasPaginas,i);
		if(tabla->pid==pidBusca) return i;
	}
	return -1; //No existe, avisa que hay que agregarla para reservarPagina
}

tabla_t* buscarTabla(int pidBusca){
	int size = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<size;i++){
		tabla_t* tabla;
		tabla = list_get(listaTablasPaginas,i);
		if(tabla->pid==pidBusca) return tabla;
	}
	return NULL; //No existe, avisa que hay que agregarla para reservarPagina
}

int existePidEnListadeTablas(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* tabla = malloc(sizeof(tabla_t));

	tabla=buscarTabla(pid);

	if(list_size(tabla->listaPaginas)==0){
		pthread_mutex_unlock(&lock_accesoTabla);
		return 0;
	}else{
		pthread_mutex_unlock(&lock_accesoTabla);
		return 1;
	}
}

int existePaginaBuscadaEnTabla(int pag, tabla_t* tablaPaginaBuscada){
	tablaPagina_t* tabla = malloc(sizeof(tablaPagina_t));
	pthread_mutex_lock(&lock_accesoTabla);
	tabla=list_get((t_list*)tablaPaginaBuscada->listaPaginas, pag);
	if(tabla){
		pthread_mutex_unlock(&lock_accesoTabla);
		return 1;
	}else{
		pthread_mutex_unlock(&lock_accesoTabla);
		return 0;
	}
}

int buscarPrimerMarcoLibre(){
	int i;
	pthread_mutex_lock(&lock_accesoMarcosOcupados);
	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			pthread_mutex_unlock(&lock_accesoMarcosOcupados);
			return i;
		}
	}
	pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	return -1;
}

int cantidadMarcosLibres(){
	int i;
	int c=0;

	pthread_mutex_lock(&lock_accesoMarcosOcupados);
	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			c++;
		}
	}
	pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	return c;
}

// FIN 0


// 1. Funciones principales

void reemplazarEntradaConLru(tablaPagina_t* pagina,int pidParam){ //Y la agrega tmb...
	pthread_mutex_lock(&lock_accesoTlb);
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
	pthread_mutex_unlock(&lock_accesoTlb);
}


void agregarATlb(tablaPagina_t* pagina,int pidParam){
	pedidoLectura_t pedido;
	pedido.pid = pidParam;
	pedido.paginaRequerida = pagina->nroPagina;
	pedido.offset=0;
	pedido.cantBytes=0;

	if(estaEnTlb(pedido)==0){
		int i;
		pthread_mutex_lock(&lock_accesoTlb);
		for(i=0;i<config.entradas_tlb;i++){
			if(tlb[i].pid==-1){
				//Se encontro un espacio libre en la tlb, vamos a guardarlo ahi
				tlb[i].pagina = pagina->nroPagina;
				tlb[i].marcoUtilizado = pagina->marcoUtilizado;
				tlb[i].pid = pidParam;
				tlb[i].contadorTiempo = tiempo++;

				pthread_mutex_unlock(&lock_accesoTlb);
				return;
			}
		}
		pthread_mutex_unlock(&lock_accesoTlb);
		reemplazarEntradaConLru(pagina,pidParam);
	}
}

int buscarEnSwap(pedidoLectura_t pedido, int cliente){
//	char* serialPID = intToChar4(pedido.pid);
//	char* serialPagina = intToChar4(pedido.paginaRequerida);
//	char* contenidoPagina = malloc(config.tamanio_marco+1);
//
//	printf("Pase por aca 1  \n");
//	enviarHeader(swapServer,HeaderOperacionLectura);
//
//	send_w(swapServer,serialPID,sizeof(int));
//	send_w(swapServer,serialPagina,sizeof(int));
//
//	printf("Pase por aca 2  \n");
//	char* header = recv_waitall_ws(swapServer,1);
//
//	printf("Pase por aca 3  \n");
//	if (charToInt(header)==HeaderOperacionLectura){
//		printf("Contesto con la pagina\n");
//	}
//	else{
//		return 0;
//	}
//
//	printf("Pase por aca 4  \n");
//	contenidoPagina = recv_waitall_ws(swapServer,config.tamanio_marco);
//
//	printf("Pase por aca 5  \n");
//	contenidoPagina[config.tamanio_marco]='\0';
//	printf("Llego el contenido de swap:%s",contenidoPagina);

	char* contenidoPagina = "SWAP";

	agregarAMemoria(pedido,contenidoPagina,cliente);


	return 1;
}



int sacarConClock(int pid){

	pthread_mutex_lock(&lock_accesoTabla);
	pthread_mutex_lock(&lock_accesoUltimaPos);

	tabla_t* tabla = malloc(sizeof(tabla_t));
	tabla = buscarTabla(pid); //TABLA CON ID A REEMPLAZAR
	int cantidadPaginas = list_size((t_list*)tabla->listaPaginas);
	int posAReemplazar;
	tablaPagina_t* puntero;

	//Primera vuelta, doy segunda oportunidad

	for(posAReemplazar=buscarUltimaPosSacada(pid); posAReemplazar<cantidadPaginas; posAReemplazar++){
			puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
			printf("Pagina q apunta puntero: %d \n",puntero->nroPagina);
			if(puntero->bitUso==0 && puntero->bitPresencia==1){
				cambiarUltimaPosicion(pid,posAReemplazar);
				pthread_mutex_unlock(&lock_accesoUltimaPos);
				pthread_mutex_unlock(&lock_accesoTabla);
				return posAReemplazar;
			}else{
				puntero->bitUso=0;
			}
		}

	//Segunda vuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}
	}

	pthread_mutex_unlock(&lock_accesoUltimaPos);
	pthread_mutex_unlock(&lock_accesoTabla);
	printf("Algo fallo en CLOCK \n");
	return -1;
}

int sacarConModificado(int pid){

	pthread_mutex_lock(&lock_accesoTabla);
	pthread_mutex_lock(&lock_accesoUltimaPos);

	tabla_t* tabla = malloc(sizeof(tabla_t));
	tabla = buscarTabla(pid);
	int cantidadPaginas = list_size((t_list*)tabla->listaPaginas);
	int posAReemplazar;
	tablaPagina_t* puntero;

	//Primera vuelta, me fijo si hay alguno (0,0) sin modificar nada
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		printf("Pos puntero: %d \n",posAReemplazar%cantidadPaginas);
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		printf("punterito puto: pag: %d pres: %d , modif %d, uso %d \n",puntero->nroPagina,puntero->bitPresencia,puntero->bitModificacion,puntero->bitUso);
		if(puntero->bitUso==0 && puntero->bitModificacion==0 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitModificacion==1 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//PrimerVuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitModificacion==1 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	pthread_mutex_unlock(&lock_accesoUltimaPos);
	pthread_mutex_unlock(&lock_accesoTabla);

	printf("Algo fallo en CLOCK MODIFICADO \n");
	return -1;
}

void sacarDeMemoria(tablaPagina_t* pagina){
		pthread_mutex_lock(&lock_accesoMemoria);
	memset(memoria+(pagina->marcoUtilizado * config.tamanio_marco),'\0',config.tamanio_marco);
		pthread_mutex_lock(&lock_accesoMarcosOcupados);
	vectorMarcosOcupados[pagina->marcoUtilizado]=0;
		pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	pagina->marcoUtilizado=-1;
		pthread_mutex_unlock(&lock_accesoMemoria);
}

void enviarASwap(int pid, tablaPagina_t* pagina){

	char* serialPID = intToChar4(pid);
	char* serialPagina = intToChar4(pagina->nroPagina);;
	char* contenidoPagina = malloc(config.tamanio_marco);
	memcpy(contenidoPagina,memoria+(pagina->marcoUtilizado * config.tamanio_marco),config.tamanio_marco);

	enviarHeader(swapServer,HeaderOperacionEscritura);
	send_w(swapServer,serialPID,sizeof(int));
	send_w(swapServer,serialPagina,sizeof(int));
	send_w(swapServer,contenidoPagina,config.tamanio_marco);
}

int cantPaginasEnMemoriaDePid(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* tablaPaginaAReemplazar = buscarTabla(pid);
	int cantidadPaginas = list_size((t_list*)tablaPaginaAReemplazar->listaPaginas);

	int i;
	int contador=0;

	for(i=0;i<cantidadPaginas;i++){
		tablaPagina_t* unaPagina;
		unaPagina = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas,i);
		if(unaPagina->bitPresencia==1){
			contador++;
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
	return contador;
}

void agregarAMemoria(pedidoLectura_t pedido, char* contenido, int cliente){
	if(cantPaginasEnMemoriaDePid(pedido.pid)>=config.marcos_x_proceso){
		int posicionPaginaSacada=0;

		if(strcmp(config.algoritmo_paginas,"CLOCK")==0){
			posicionPaginaSacada=sacarConClock(pedido.pid);
		}else {
			if(strcmp(config.algoritmo_paginas,"CLOCK_MODIFICADO")==0){
				posicionPaginaSacada=sacarConModificado(pedido.pid);
			}
			else{
				printf("Error sintaxis algoritmo: CLOCK o CLOCK_MODIFICADO");
			}

		}
		printf("PAGINA A SACAR DE MEMORIA: %d \n ", posicionPaginaSacada);

		pthread_mutex_lock(&lock_accesoTabla);

		tabla_t* tablaPaginaAReemplazar = buscarTabla(pedido.pid);
		tablaPagina_t* paginaASacarDeMemoria = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas, posicionPaginaSacada);
		pthread_mutex_unlock(&lock_accesoTabla);

		enviarASwap(pedido.pid,paginaASacarDeMemoria);
		sacarDeMemoria(paginaASacarDeMemoria);

		paginaASacarDeMemoria->bitPresencia=0;
		paginaASacarDeMemoria->marcoUtilizado=-1;

		pthread_mutex_lock(&lock_accesoTabla);
		tablaPagina_t* paginaACargar = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas, pedido.paginaRequerida);
		pthread_mutex_unlock(&lock_accesoTabla);

		int unMarcoNuevo = buscarPrimerMarcoLibre();
		pthread_mutex_lock(&lock_accesoMarcosOcupados);
		vectorMarcosOcupados[unMarcoNuevo]=1; //Lo marco como ocupado
		pthread_mutex_unlock(&lock_accesoMarcosOcupados);

		paginaACargar->marcoUtilizado = unMarcoNuevo;
		paginaACargar->bitPresencia = 1;
		paginaACargar->bitModificacion = 0;
		paginaACargar->bitUso=1;

		flushTlb();
		almacenarBytesEnUnaPagina(pedido,config.tamanio_marco,contenido,cliente);
	}
	else{
		pthread_mutex_lock(&lock_accesoTabla);
		tabla_t* tablaPaginaAReemplazar = buscarTabla(pedido.pid);
		tablaPagina_t* paginaACargar = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas,pedido.paginaRequerida);
		pthread_mutex_unlock(&lock_accesoTabla);
		int unMarcoNuevo = buscarPrimerMarcoLibre();
		pthread_mutex_lock(&lock_accesoMarcosOcupados);
		vectorMarcosOcupados[unMarcoNuevo]=1; //Lo marco como ocupado
		pthread_mutex_unlock(&lock_accesoMarcosOcupados);

		paginaACargar->marcoUtilizado = unMarcoNuevo;
		paginaACargar->bitPresencia = 1;
		paginaACargar->bitModificacion = 0;
		paginaACargar->bitUso = 1;

		almacenarBytesEnUnaPagina(pedido, config.tamanio_marco, contenido, cliente);
	}

}

char* devolverPedidoPagina(pedidoLectura_t pedido, int cliente){

	log_info(activeLogger,"LECTURA DE pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

//SI ESTA EN TLB DEVUELVO
	if(estaEnTlb(pedido) && config.entradas_tlb){

		log_info(activeLogger,"Se encontro en la Tlb el pid: %d, pagina: %d PARA LECTURA \n",pedido.pid,pedido.paginaRequerida);
		int pos = buscarEnTlb(pedido);

		char* contenido = malloc(pedido.cantBytes + 1);

		printf("Accediendo a memoria... \n");
		usleep(retardoMemoria);

		pthread_mutex_lock(&lock_accesoMemoria);
		memcpy(contenido,memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);
		contenido[pedido.cantBytes]='\0';
		pthread_mutex_unlock(&lock_accesoMemoria);

		printf("marco tlb: %d \n", tlb[pos].marcoUtilizado);
		return contenido;

	}
//SINO, ME FIJO QUE SEA VALIDA LA PETICION
	else{
		log_info(activeLogger,"No se encontro en la Tlb el pid: %d, pagina: %d. Se buscara en la Lista de tablas de paginas",pedido.pid,pedido.paginaRequerida);
		if(existePidEnListadeTablas(pedido.pid)){ //Si existe la tabla de paginas dentro de la lista
			pthread_mutex_lock(&lock_accesoTabla);
			tabla_t* tablaPaginaBuscada = buscarTabla(pedido.pid);
			pthread_mutex_unlock(&lock_accesoTabla);
			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				pthread_mutex_lock(&lock_accesoTabla);
				tablaPagina_t* paginaBuscada = list_get((t_list*)tablaPaginaBuscada->listaPaginas, pedido.paginaRequerida);
//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB

				printf("Accediendo a memoria... \n"); //Accedo a memoria para leer la tabla
				usleep(retardoMemoria);

				if(paginaBuscada->bitPresencia){
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger,"Se encontro la pagina y esta en memoria! Devolviendo pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

					printf("Accediendo a memoria... \n");
					usleep(retardoMemoria);

					char* contenido = malloc(pedido.cantBytes + 1);
					pthread_mutex_lock(&lock_accesoMemoria);
					memcpy(contenido,memoria+paginaBuscada->marcoUtilizado * config.tamanio_marco+pedido.offset,pedido.cantBytes);
					contenido[pedido.cantBytes]='\0';
					pthread_mutex_unlock(&lock_accesoMemoria);

					printf("marco tlb: %d \n", paginaBuscada->marcoUtilizado);

					agregarATlb(paginaBuscada,pedido.pid);

					return contenido;

				}
// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y VUELVO A LLAMAR A FUNCION
				else{
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger,"Se encontro la pagina pero NO esta en memoria (LECTURA)! Buscando en swap: pag:%d de pid:%d \n",pedido.paginaRequerida,pedido.pid);

					int pudo = buscarEnSwap(pedido, cliente);

					if(pudo){
						agregarATlb(paginaBuscada,pedido.pid);
						log_info(activeLogger,"Cargada pagina en memoria, agregada a TLB, se vuelve a hacer el pedido de lectura! Devolviendo pag:%d de pid:%d \n",pedido.paginaRequerida,pedido.pid);
						devolverPedidoPagina(pedido, cliente);
					}
					else{
						return "Error busqueda en swap";
					}
				}
			}
// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
				send_w(clientes[cliente].socket, intToChar(HeaderNoExistePagina), 4);
			}
		}
// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		else{
			send_w(clientes[cliente].socket,intToChar(HeaderNoExisteTablaDePag), 4);
		}
	}
	return NULL;
}

int paginasQueOcupa(char* contenido){
	return (strlen(contenido) + config.tamanio_marco - 1) / config.tamanio_marco;
}



int inicializarPrograma(int idPrograma, char* contenido){

	char* serialPID = intToChar4(idPrograma);
	int cantidadPags = paginasQueOcupa(contenido);
	char* serialCantidadPaginas = intToChar4(cantidadPags);
	int i;

	enviarHeader(swapServer,HeaderOperacionIniciarProceso);
	send_w(swapServer,serialPID,sizeof(int));
	send_w(swapServer,serialCantidadPaginas,sizeof(int));

	for(i=0;i<cantidadPags;i++){
		char* fraccionCodigo = malloc(config.tamanio_marco);
		memcpy(&fraccionCodigo,contenido+(i*config.tamanio_marco),config.tamanio_marco);
		send_w(swapServer,intToChar4(strlen(fraccionCodigo)),sizeof(int)); //Le mando el tamanio de la fraccion porque la ultima no esta completa
		send_w(swapServer,fraccionCodigo,strlen(fraccionCodigo));
	}

	char* header = recv_waitall_ws(swapServer,1);

	if (charToInt(header)==HeaderProcesoAgregado){
		printf("Contesto el proceso Agregado\n");
		return 1;
	}else{
		return 0;
	}
}

char* almacenarBytesEnUnaPagina(pedidoLectura_t pedido, int size, char* buffer,int cliente){

	log_info(activeLogger,"ESCRITURA DE pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

	if(estaEnTlb(pedido) && config.entradas_tlb){

		log_info(activeLogger,"Se encontro en la Tlb el pid: %d, pagina: %d PARA ESCRITURA \n",pedido.pid,pedido.paginaRequerida);

		int pos = buscarEnTlb(pedido);

		printf("Posicion encontrada en la TLB: %d \n \n",pos);

		printf("Accediendo a memoria... \n ");
		usleep(retardoMemoria);

		pthread_mutex_lock(&lock_accesoMemoria);
		memcpy(memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, buffer, strlen(buffer)); //size??? PARA QUE??
		pthread_mutex_unlock(&lock_accesoMemoria);

		printf("marco tlb: %d \n", tlb[pos].marcoUtilizado);

		printf("Lo que acabo de almacenar: %s .\n \n ",memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset);
		printf("Ahora llamo a la funcion devolverPedidoPagina (conecto las dos func) \n \n");
		return (devolverPedidoPagina(pedido,cliente)); //Provisorio para testear

	}
	else{
		log_info(activeLogger,"No se encontro en la Tlb el pid: %d, pagina: %d. Se buscara en la Lista de tablas de paginas \n",pedido.pid,pedido.paginaRequerida);
		printf("PID: %d \n",pedido.pid);
		if(existePidEnListadeTablas(pedido.pid)){ //Si existe la tabla de paginas dentro de la lista
			pthread_mutex_lock(&lock_accesoTabla);
			tabla_t* tablaPaginaBuscada = buscarTabla(pedido.pid);
			pthread_mutex_unlock(&lock_accesoTabla);

			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				pthread_mutex_lock(&lock_accesoTabla);
				tablaPagina_t* paginaBuscada = list_get((t_list*)tablaPaginaBuscada->listaPaginas, pedido.paginaRequerida);

	//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB

				printf("Accediendo a memoria... \n"); //Accedo a memoria para leer la tabla
				usleep(retardoMemoria);

				if(paginaBuscada->bitPresencia){
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger,"Se encontro la pagina y esta en memoria! Escribiendo pag:%d de pid:%d \n",pedido.paginaRequerida,pedido.pid);

					printf("Accediendo a memoria... \n ");
					usleep(retardoMemoria);

					pthread_mutex_lock(&lock_accesoTabla);
					paginaBuscada->bitModificacion = 1;  //NEW
					pthread_mutex_unlock(&lock_accesoTabla);

					pthread_mutex_lock(&lock_accesoMemoria);
					memcpy(memoria+paginaBuscada->marcoUtilizado*config.tamanio_marco+pedido.offset, buffer, strlen(buffer)); //size??? PARA QUE??
					pthread_mutex_unlock(&lock_accesoMemoria);

					printf("Marco de la pagina: %d \n", paginaBuscada->marcoUtilizado);

					printf("Lo que acabo de almacenar: %s .\n \n ",memoria+paginaBuscada->marcoUtilizado*config.tamanio_marco+pedido.offset);
					printf("Ahora llamo a la funcion devolverPedidoPagina (conecto las dos func) \n \n");

					agregarATlb(paginaBuscada,pedido.pid);

					//send_w(cliente, devolucion, 4);
					return (devolverPedidoPagina(pedido,cliente)); //Provisorio para testear
				}
	// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y RECIEN AHI LA DEVUELVOl, SI NO HAY PAGINAS DISPONIBLES: ALGORITMO DE SUSTITUCION DE PAGINAS
				else{
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger,"Se encontro la pagina pero NO esta en memoria (ESCRITURA)! Buscando en swap: pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

					int pudo = buscarEnSwap(pedido,cliente);
					if(pudo){
						agregarATlb(paginaBuscada,pedido.pid);
						log_info(activeLogger,"Cargada pagina en memoria, agregada a TLB, se vuelve a hacer el pedido de escritura! Devolviendo pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);
						devolverPedidoPagina(pedido,cliente);
					}
					else{
						return "Error busqueda en swap";
					}
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

	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* auxiliar = buscarTabla(idPrograma);

	tablaPagina_t* tabla = malloc(sizeof(tablaPagina_t));
	int i;
	int size = list_size((t_list*)auxiliar->listaPaginas);
	for(i=0;i<size;i++){
		tabla = list_get((t_list*)auxiliar->listaPaginas,i);

		pthread_mutex_lock(&lock_accesoMarcosOcupados);
		vectorMarcosOcupados[tabla->marcoUtilizado] = 0;
		pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	}
	pthread_mutex_unlock(&lock_accesoTabla);
}

void finalizarPrograma(int idPrograma){
	enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
	send_w(swapServer,intToChar4(idPrograma),sizeof(int));
	sacarMarcosOcupados(idPrograma);
	flushTlb();
	tabla_t* tabla = buscarTabla(idPrograma);
	list_destroy((t_list*)tabla->listaPaginas);
	list_remove(listaTablasPaginas,buscarPosicionTabla(idPrograma));
}

//FIN 1


//2. Funciones que se mandan por consola

void devolverTodasLasPaginas(){  //OK
	pthread_mutex_lock(&lock_accesoTabla);
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		tabla_t* unaTabla = malloc(sizeof(tabla_t));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get((t_list*)unaTabla->listaPaginas,j);

			printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",unaTabla->pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
			log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",unaTabla->pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
}

void devolverPaginasDePid(int pid){ //OK
	tabla_t* unaTabla = malloc(sizeof(tabla_t));

	if(existePidEnListadeTablas(pid)){

			pthread_mutex_lock(&lock_accesoTabla);
			unaTabla = buscarTabla(pid);

			int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
			int i;

			for(i=0;i<cantidadPaginasDeTabla;i++){
				tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
				unaPagina = list_get((t_list*)unaTabla->listaPaginas,i);
				printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
				log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
			}
	}else{
		printf("Ese pid no existe o no esta en uso! \n");
	}
	pthread_mutex_unlock(&lock_accesoTabla);
}

void imprimirRegionMemoria(char* region, int size){
	int i;
	for(i=0;i<size;i++){
			putchar(region[i]);
	}
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

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get((t_list*)unaTabla->listaPaginas,j);
			//Hago un solo print f de las caracteristicas
			printf("Accediendo a memoria... \n ");
			usleep(retardoMemoria);

			printf("Pid: %d, Pag: %d, Marco: %d, Contenido: ",unaTabla->pid, unaPagina->nroPagina,unaPagina->marcoUtilizado);

			if(unaPagina->bitPresencia==1){
				pthread_mutex_lock(&lock_accesoMemoria);
				char* contenido = malloc(config.tamanio_marco+1);
				memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
				contenido[config.tamanio_marco]='\0';
				pthread_mutex_unlock(&lock_accesoMemoria);

				imprimirRegionMemoria(contenido,config.tamanio_marco);
			}

			pthread_mutex_lock(&lock_accesoMemoria);
			char* contenido = malloc(config.tamanio_marco+1);
			memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
			contenido[config.tamanio_marco]='\0';
			pthread_mutex_unlock(&lock_accesoMemoria);

			log_info(dump,"Pid: %d, Pag: %d, Marco: %d, Contenido: %s ",unaTabla->pid, unaPagina->nroPagina,unaPagina->marcoUtilizado,contenido);

			printf("\n");
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
	printf("\n");
}

void devolverMemoriaDePid(int pid){ //OK
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* unaTabla = malloc(sizeof(tabla_t));

	unaTabla = buscarTabla(pid);
	int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
	int i;

	for(i=0;i<cantidadPaginasDeTabla;i++){
		tablaPagina_t* unaPagina;// = malloc(sizeof(tablaPagina_t));
		unaPagina = list_get((t_list*)unaTabla->listaPaginas,i);

		if(unaPagina->bitPresencia==1){

			printf("Accediendo a memoria... \n ");
			usleep(retardoMemoria);

			printf("Pid: %d, Pag: %d, Marco: %d, Contenido: ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado);

			pthread_mutex_lock(&lock_accesoMemoria);
			char* contenido = malloc(config.tamanio_marco+1);
			memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
			contenido[config.tamanio_marco]='\0';
			pthread_mutex_unlock(&lock_accesoMemoria);

			imprimirRegionMemoria(contenido,config.tamanio_marco);
			log_info(dump, "Pid: %d, Pag: %d, Marco: %d, Contenido: %s ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,contenido);

			printf("\n");
		}
		else{
			printf("La pagina: %d del pid: %d no esta en memoria \n",unaPagina->nroPagina,pid);
		}
	}
	printf("\n");

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
			devolverTodasLasPaginas();
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
void inicializarTlb(){
	pthread_mutex_lock(&lock_accesoTlb);
	tiempo=0;
	int i;
	for(i = 0; i<config.entradas_tlb; i++){
		tlb[i].pid=-1;
		tlb[i].pagina=-1;
		tlb[i].marcoUtilizado=-1;
		tlb[i].contadorTiempo=-1;
	}
	pthread_mutex_unlock(&lock_accesoTlb);
}
void flushTlb(){
	inicializarTlb();
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


	vectorClientes = malloc(MAXCLIENTS * sizeof(int));
	memset(vectorClientes,-1, MAXCLIENTS * sizeof(int));

	retardoMemoria = config.retardo;

	listaUltimaPosicionSacada = list_create();

//	int i;
//	for(i=0;i<MAXCLIENTS;i++){
//		vectorUltimaPosicionSacada[i]=0;
//	}

	pthread_attr_init(&detachedAttr);
	pthread_attr_setdetachstate(&detachedAttr, PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&lock_accesoMarcosOcupados, NULL);
	pthread_mutex_init(&lock_accesoLog, NULL);
	pthread_mutex_init(&lock_accesoMemoria, NULL);
	pthread_mutex_init(&lock_accesoTabla, NULL);
	pthread_mutex_init(&lock_accesoTlb, NULL);
	pthread_mutex_init(&lock_accesoUltimaPos, NULL);

}
// FIN 3

// 4. Procesar headers

int primerNumeroPaginaLibre(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* auxiliar = buscarTabla(pid);
	pthread_mutex_unlock(&lock_accesoTabla);
	return list_size((t_list*)auxiliar->listaPaginas);
}

int reservarPagina(int cantPaginasPedidas, int pid) { // OK

	if(buscarTabla(pid)==NULL){
		tabla_t* tabla = malloc(sizeof(tabla_t));
		tabla->pid=pid;
		tabla->listaPaginas=list_create();
		list_add(listaTablasPaginas,tabla);

		ultimaSacada_t* entrada = malloc(sizeof(ultimaSacada_t));
		entrada->pid=pid;
		entrada->posicion=0;
		list_add(listaUltimaPosicionSacada,entrada);
	}

	int i;
	for (i = 0; i < cantPaginasPedidas; i++) {
		tablaPagina_t *nuevaPag = malloc(sizeof(tablaPagina_t));
		int posicion;
		if (existePidEnListadeTablas(pid)) {
			posicion = primerNumeroPaginaLibre(pid);
		} else {
			posicion = i;
		}

		pthread_mutex_lock(&lock_accesoTabla);
		nuevaPag->nroPagina = posicion;
		nuevaPag->marcoUtilizado = -1;
		nuevaPag->bitPresencia = 0;
		nuevaPag->bitModificacion = 0;
		nuevaPag->bitUso = 0;

		tabla_t* tablaPag = buscarTabla(pid);
		list_add_in_index((t_list*)tablaPag->listaPaginas, posicion, nuevaPag);
		pthread_mutex_unlock(&lock_accesoTabla);
	}
	return 1;
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

void pedidoLectura(int cliente){

	t_pedido* pedidoCpu = malloc(sizeof(t_pedido));
	char* pedidoSerializado = malloc(sizeof(t_pedido));
	int id = clientes[cliente].pid;

	log_info(activeLogger,"Se recibio pedido de lectura de id: %d \n",id);

	read(clientes[cliente].socket, pedidoSerializado, sizeof(t_pedido));

	imprimir_serializacion(pedidoSerializado,12);
	deserializar_pedido(pedidoCpu, pedidoSerializado);

	pedidoLectura_t pedidoLectura;
	pedidoLectura.pid = id;
	pedidoLectura.paginaRequerida = pedidoCpu->pagina;
	pedidoLectura.offset = pedidoCpu->offset;
	pedidoLectura.cantBytes = pedidoCpu->size;


	char* contenidoAEnviar =  devolverPedidoPagina(pedidoLectura,cliente);
	printf("Contenido a enviado a Cpu: %s \n", contenidoAEnviar);
	send_w(clientes[cliente].socket, contenidoAEnviar,pedidoCpu->size);
}

void headerEscribirPagina(int cliente){
	t_pedido* pedidoCpuEscritura = malloc(sizeof(t_pedido));
	char* pedidoSerializadoEscritura = malloc(sizeof(t_pedido));
	int id = clientes[cliente].pid;
	char* buffer = malloc(sizeof(int));

	read(clientes[cliente].socket, pedidoSerializadoEscritura, sizeof(t_pedido));
	deserializar_pedido(pedidoCpuEscritura,pedidoSerializadoEscritura);
	read(clientes[cliente].socket, buffer, sizeof(int));

	pedidoLectura_t pedido;
	pedido.pid = id;
	pedido.paginaRequerida = pedidoCpuEscritura->pagina;
	pedido.offset = pedidoCpuEscritura->offset;
	pedido.cantBytes = pedidoCpuEscritura->size;

	almacenarBytesEnUnaPagina(pedido,strlen(buffer),buffer,cliente);
}

void procesarHeader(int cliente, char *header){
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger,"Llego un mensaje con header %d\n",charToInt(header));
	clientes[cliente].atentido=true;

	char* pidScript = NULL;
	char* cantidadDePaginasScript = NULL;
	char* tamanioCodigoScript = NULL;
	char* codigoScript = NULL;

	switch(charToInt(header)) {

	case HeaderError:
		log_error(activeLogger,"Header de Error\n");
		clientes[cliente].atentido=false;
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
//			pthread_create(&(vectorHilosCpu[clientes[cliente].pid]),&detachedAttr,(void*)esperar_header,(void*)cliente);
//			crearHiloConParametro(&hiloParaCpu,(void*)esperar_header ,(void*)cliente);
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

		case HeaderTamanioPagina:
			printf("Pedido tamanio de paginas \n");
			send_w(clientes[cliente].socket,intToChar4(config.tamanio_marco),sizeof(int));
			clientes[cliente].atentido=false;
			break;

		case HeaderPedirValorVariable:  //PARA NUCLEO
			pedidoLectura(cliente);
			clientes[cliente].atentido=false;
			break;

		case HeaderSolicitudSentencia: //PARA CPU
			crearHiloConParametro(&hiloParaCpu,(void*)pedidoLectura,(void*)cliente);
			clientes[cliente].atentido=false;
			break;

		case HeaderScript: //Inicializar programa  // OK
			read(clientes[cliente].socket , cantidadDePaginasScript, 4);
			read(clientes[cliente].socket , tamanioCodigoScript, 4);
			read(clientes[cliente].socket , pidScript, 4);
			read(clientes[cliente].socket , codigoScript, atoi(tamanioCodigoScript));

			if(inicializarPrograma(atoi(pidScript),codigoScript)){
				reservarPagina(atoi(cantidadDePaginasScript),atoi(pidScript));
				send_w(clientes[cliente].socket,"1",sizeof(int));
			}else{
				send_w(clientes[cliente].socket,"0",sizeof(int));
			}
			clientes[cliente].atentido=false;
			break;

		case HeaderAsignarValor: // CPU
			log_info(activeLogger,"Se recibio pedido de grabar una pagina, por CPU");
			crearHiloConParametro(&hiloParaCpu,(void*)headerEscribirPagina,(void*)cliente);
			clientes[cliente].atentido=false;
			break;

		case HeaderLiberarRecursosPagina:
			log_info(activeLogger,"Se recibio pedido de liberar una pagina, por CPU");
			char* pidALiberar = malloc(sizeof(int));
			read(clientes[cliente].socket , pidALiberar, sizeof(int));
			finalizarPrograma(atoi(pidALiberar));
			clientes[cliente].atentido=false;
			break;

		default:
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
	pthread_mutex_destroy(&lock_accesoMarcosOcupados);
	pthread_mutex_destroy(&lock_accesoMemoria);
	pthread_mutex_destroy(&lock_accesoTabla);
	pthread_mutex_destroy(&lock_accesoTlb);
	pthread_mutex_destroy(&lock_accesoUltimaPos);
}


int main(void) { //campo pid a tabla paginas, y en vez de list_get buscarRecursivo

	cargarCFG();

	crearLogs("Umc","Proceso",0);

	dump = log_create("dump","UMC",false,LOG_LEVEL_INFO);

	log_info(activeLogger,"Soy umc de process ID %d.\n", getpid());

	listaTablasPaginas = list_create();

//	int k;
//	for(k=0;k<config.cantidad_marcos;k++){  //COMO MAXIMO ES LA CANTIDAD DE MARCOS, considerando q como minimo una tabla tiene 1 pag
//		tabla_t* tablaPaginas;
//		tablaPaginas->listaPaginas = list_create();
//		list_add(listaTablasPaginas,tablaPaginas);
//	}

	crearMemoriaYTlbYTablaPaginas();

	test2();

//	recibirComandos();

//	pthread_create(&hiloRecibirComandos,NULL,(void*)recibirComandos,NULL);

//	servidorCPUyNucleoExtendido();

//
//	conexionASwap();
//
//	pedidoLectura_t pedido1;
//			pedido1.pid=2;
//			pedido1.paginaRequerida=1;
//			pedido1.offset=0;
//			pedido1.cantBytes=5;
//
//	buscarEnSwap(pedido1);

//	recibirComandos();



	finalizar();

	return 0;
}























void mostrarTlb(){
	int i;
	printf("\n -- TLB -- \n");
	for(i=0;i<config.entradas_tlb;i++){
		tlb_t unaEntrada;
		unaEntrada = tlb[i];

		printf("Pos: %d Pid: %d Pag: %d Marco: %d Contador: %d \n",i,unaEntrada.pid,unaEntrada.pagina,unaEntrada.marcoUtilizado,unaEntrada.contadorTiempo);

	}
}

void test2(){

	reservarPagina(3,-3);

	pedidoLectura_t pedido1;
		pedido1.pid=-3;
		pedido1.paginaRequerida=1;
		pedido1.offset=0;
		pedido1.cantBytes=5;

	devolverPedidoPagina(pedido1,0);

	pedidoLectura_t pedido2;
		pedido2.pid=-3;
		pedido2.paginaRequerida=2;
		pedido2.offset=0;
		pedido2.cantBytes=5;

	devolverPedidoPagina(pedido2,0);

	mostrarTlb();

	almacenarBytesEnUnaPagina(pedido2,2,"XX",0);

	printf("CANT PAGS PID 0 EN MEM: %d \n", cantPaginasEnMemoriaDePid(-3));

	reservarPagina(3,0);

		pedidoLectura_t pedido3;
			pedido3.pid=0;
			pedido3.paginaRequerida=1;
			pedido3.offset=0;
			pedido3.cantBytes=5;

		devolverPedidoPagina(pedido3,0);

		pedidoLectura_t pedido4;
			pedido4.pid=0;
			pedido4.paginaRequerida=2;
			pedido4.offset=0;
			pedido4.cantBytes=5;

		devolverPedidoPagina(pedido4,0);

	reservarPagina(3,-3);

	recibirComandos();
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

//	char* umcLog = "UMC";
//	char* procLog = "Proceso";
//	crearLogs(umcLog,procLog,NULL);
//	crearLog("Umc","Uasmc");

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

void ejemploSWAP(){
	// DATOS EL PROCESO
		char* serialPID = intToChar4(1);
		char* serialCantidadPaginas = intToChar4(5);
		char* serialPagina = intToChar4(2);;
		char* contenidoPagina = malloc(config.tamanio_marco);
		contenidoPagina = "123456789";

		// INICIAR PROCESO
		enviarHeader(swapServer,HeaderOperacionIniciarProceso);
		send_w(swapServer,serialPID,sizeof(int));
		send_w(swapServer,serialCantidadPaginas,sizeof(int));
		char* header = recv_waitall_ws(swapServer,1);
		if (charToInt(header)==HeaderProcesoAgregado)
			printf("Contesto el proceso Agregado\n");

		// ESCRIBIR PAGINA
		enviarHeader(swapServer,HeaderOperacionEscritura);
		send_w(swapServer,serialPID,sizeof(int));
		send_w(swapServer,serialPagina,sizeof(int));
		send_w(swapServer,contenidoPagina,config.tamanio_marco);

		// LEER PAGINA
		enviarHeader(swapServer,HeaderOperacionLectura);
		char* contenidoPagina2 = malloc(config.tamanio_marco+1);
		send_w(swapServer,serialPID,sizeof(int));
		send_w(swapServer,serialPagina,sizeof(int));
		header = recv_waitall_ws(swapServer,1);
		if (charToInt(header)==HeaderOperacionLectura)
			printf("Contesto con la pagina\n");
		contenidoPagina2[config.tamanio_marco]='\0';
		printf("Llego el msg:%s",contenidoPagina2);
		contenidoPagina2 = recv_waitall_ws(swapServer,config.tamanio_marco);
		printf("Llego el contenido y es igual:%d\n",strcmp(contenidoPagina,contenidoPagina2)==0);

		// FINALIZAR PROCESO
		enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
		send_w(swapServer,serialPID,sizeof(int));

}

void conexionASwap(){ //Creada para unir las dos funciones y crear un hilo
	realizarConexionASwap();
//	ejemploSWAP();
	escucharPedidosDeSwap();

}

// FIN 6
