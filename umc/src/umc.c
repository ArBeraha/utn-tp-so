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
				//Se encontro un espacio libre en la tlb, se va a guardar ahi
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

int buscarEnSwap(pedidoLectura_t pedido, t_cliente cliente){
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


	if(pedido.paginaRequerida==1){
		char* contenidoPagina = "abcdefghijklmnopqrstuvwxyz";
		agregarAMemoria(pedido,contenidoPagina,cliente);
	}
	if(pedido.paginaRequerida==3){

		char* str = malloc(sizeof(int));
		memcpy(str,"4",4);
		agregarAMemoria(pedido,str,cliente);
	}

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
	log_error(activeLogger, "[%d] Algo fallo en CLOCK",pid);
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
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
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

	log_error(activeLogger, "[%d] Algo fallo en CLOCK MODIFICADO",pid);

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

int cantPaginasDePid(int pid){
	tabla_t* tablaPaginaAReemplazar = buscarTabla(pid);
	return list_size((t_list*)tablaPaginaAReemplazar->listaPaginas);
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

void agregarAMemoria(pedidoLectura_t pedido, char* contenido, t_cliente cliente){
	int id=0;
//	MUTEXCLIENTES(id=clientes[cliente.identidad].pid);
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
		log_info(activeLogger, "[%d] No hay marcos disponibles. [Pag] Sacando [%d]. Agregando [%d]. Con: %s",id,posicionPaginaSacada,pedido.paginaRequerida, config.algoritmo_paginas);

		pthread_mutex_lock(&lock_accesoTabla);
		tabla_t* tablaPaginaAReemplazar = buscarTabla(pedido.pid);
		tablaPagina_t* paginaASacarDeMemoria = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas, posicionPaginaSacada);
		pthread_mutex_unlock(&lock_accesoTabla);

		if(paginaASacarDeMemoria->bitModificacion){
			enviarASwap(pedido.pid,paginaASacarDeMemoria);
		}

		int marcoSacado = paginaASacarDeMemoria->marcoUtilizado;
		log_info(activeLogger, "[%d] Intercambiando contenido de Marco [%d]",id,marcoSacado);

		sacarDeMemoria(paginaASacarDeMemoria);

		paginaASacarDeMemoria->bitPresencia=0;
		paginaASacarDeMemoria->marcoUtilizado=-1;

		pthread_mutex_lock(&lock_accesoTabla);
		tablaPagina_t* paginaACargar = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas, pedido.paginaRequerida);
		pthread_mutex_unlock(&lock_accesoTabla);

		pthread_mutex_lock(&lock_accesoMarcosOcupados);
		vectorMarcosOcupados[marcoSacado]=1; //Lo marco como ocupado porque va a ir el nuevo proceso ahi
		pthread_mutex_unlock(&lock_accesoMarcosOcupados);

		paginaACargar->marcoUtilizado = marcoSacado;
		paginaACargar->bitPresencia = 1;
		paginaACargar->bitModificacion = 0;
		paginaACargar->bitUso=1;

		flushTlb();
		pedido.cantBytes=config.tamanio_marco;
		pedido.offset=0;
		almacenarBytesEnUnaPagina(pedido,contenido,cliente);
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

		pedido.cantBytes=config.tamanio_marco;
		pedido.offset=0;
		almacenarBytesEnUnaPagina(pedido, contenido, cliente);
	}

}

char* devolverPedidoPagina(pedidoLectura_t pedido, t_cliente cliente){

//SI ESTA EN TLB DEVUELVO
	int id =0;
//	MUTEXCLIENTES(id = clientes[cliente.identidad].pid);


	if(estaEnTlb(pedido) && config.entradas_tlb){

		int pos = buscarEnTlb(pedido);

		char* contenido = malloc(pedido.cantBytes);

		log_info(activeLogger, "[%d][L] Accediendo a MP",id);
		usleep(retardoMemoria);

		pthread_mutex_lock(&lock_accesoMemoria);
		memcpy(contenido,memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);

		pthread_mutex_unlock(&lock_accesoMemoria);

		log_info(activeLogger, "[%d][L] Se encontro en TLB para LECTURA [Pag,Off,Bytes] = [%d,%d,%d] en MARCO: %d",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes,tlb[pos].marcoUtilizado);

		return contenido;

	}
//SINO, ME FIJO QUE SEA VALIDA LA PETICION
	else{
		if(existePidEnListadeTablas(pedido.pid)){ //Si existe la tabla de paginas dentro de la lista
			pthread_mutex_lock(&lock_accesoTabla);
			tabla_t* tablaPaginaBuscada = buscarTabla(pedido.pid);
			pthread_mutex_unlock(&lock_accesoTabla);
			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				pthread_mutex_lock(&lock_accesoTabla);
				tablaPagina_t* paginaBuscada = list_get((t_list*)tablaPaginaBuscada->listaPaginas, pedido.paginaRequerida);
//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB

				log_info(activeLogger, "[%d][L] Accediendo a MP",id);
				usleep(retardoMemoria);

				if(paginaBuscada->bitPresencia){
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger, "[%d][L] Se encontro en Tabla de Paginas y esta en memoria",id);
					log_info(activeLogger, "[%d][L] Realizando LECTURA [Pag,Off,Bytes] = [%d,%d,%d]",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes);

					log_info(activeLogger, "[%d][L] Accediendo a MP",id);
					usleep(retardoMemoria);

					char* contenido = malloc(pedido.cantBytes);
					pthread_mutex_lock(&lock_accesoMemoria);
					memcpy(contenido,memoria+paginaBuscada->marcoUtilizado * config.tamanio_marco+pedido.offset,pedido.cantBytes);
					pthread_mutex_unlock(&lock_accesoMemoria);

					agregarATlb(paginaBuscada,pedido.pid);

					log_info(activeLogger, "[%d][L] Agregado a TLB [Pagina,Marco] = [%d,%d]",id,pedido.paginaRequerida,paginaBuscada->marcoUtilizado);

					return contenido;

				}
// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y VUELVO A LLAMAR A FUNCION
				else{
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger, "[%d][L] Se encontro en Tabla de Paginas pero NO ESTA EN MEMORIA. Buscando en SWAP: [Pag]=[%d]",id,pedido.paginaRequerida);
					log_info(activeLogger, "[%d][L]-------------SWAP-----------",id);

					int pudo = buscarEnSwap(pedido, cliente);

					if(pudo){
						agregarATlb(paginaBuscada,pedido.pid);
						log_info(activeLogger, "[%d][L] Se encontro en SWAP [Pag]=[%d] y se agrego a memoria. Realizando pedido de LECTURA nuevamente",id,pedido.paginaRequerida);
						log_info(activeLogger, "[%d][L]---------------------------",id);
						devolverPedidoPagina(pedido, cliente);
					}
					else{
						log_info(activeLogger, "[%d][L] NO se encontro en SWAP [Pag]=[%d]",id,pedido.paginaRequerida);
						return "Error busqueda en swap";
					}
				}
			}
// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
				enviarHeader(cliente.socket,HeaderNoExistePagina);
			}
		}
// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		else{
			enviarHeader(cliente.socket,HeaderNoExisteTablaDePag);
		}
	}
	return NULL;
}

int paginasQueOcupa(int tamanio){
	return (tamanio + config.tamanio_marco - 1) / config.tamanio_marco;
}



int inicializarPrograma(int idPrograma, char* contenido,int tamanio){

	char* serialPID = intToChar4(idPrograma);
	int cantidadPags = paginasQueOcupa(tamanio);
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

void ponerBitModif1(int pid,int pag){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* tabla = buscarTabla(pid);
	tablaPagina_t* pagina = list_get((t_list*)tabla->listaPaginas,pag);
	pagina->bitModificacion=1;
	pthread_mutex_unlock(&lock_accesoTabla);
}

char* almacenarBytesEnUnaPagina(pedidoLectura_t pedido, char* buffer,t_cliente cliente){

	int id =0;
//	MUTEXCLIENTES(id=clientes[cliente.identidad].pid);

	if(estaEnTlb(pedido) && config.entradas_tlb){

		int pos = buscarEnTlb(pedido);

		log_info(activeLogger, "[%d][E] Accediendo a MP",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes,tlb[pos].marcoUtilizado);
		usleep(retardoMemoria);

		pthread_mutex_lock(&lock_accesoMemoria);
		memcpy(memoria+(tlb[pos].marcoUtilizado*config.tamanio_marco)+pedido.offset, buffer, pedido.cantBytes);
		pthread_mutex_unlock(&lock_accesoMemoria);

		ponerBitModif1(pedido.pid,pedido.paginaRequerida);

		log_info(activeLogger, "[%d][E] Se encontro en TLB para ESCRITURA [Pag,Off,Bytes] = [%d,%d,%d] en MARCO: %d",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes,tlb[pos].marcoUtilizado);
		log_info(activeLogger, "[%d][E] Se almaceno: ",id);
		if(pedido.paginaRequerida<=cantPaginasDePid(pedido.pid)- paginas_stack){
			imprimirRegionMemoriaCodigo(memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);
//			imprimirRegionMemoriaStack(memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);

		}else{
			imprimirRegionMemoriaStack(memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);
		}

		return "1";
	}
	else{
		if(existePidEnListadeTablas(pedido.pid)){
			pthread_mutex_lock(&lock_accesoTabla);
			tabla_t* tablaPaginaBuscada = buscarTabla(pedido.pid);
			pthread_mutex_unlock(&lock_accesoTabla);

			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				pthread_mutex_lock(&lock_accesoTabla);
				tablaPagina_t* paginaBuscada = list_get((t_list*)tablaPaginaBuscada->listaPaginas, pedido.paginaRequerida);

	//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB

				log_info(activeLogger, "[%d][E] Accediendo a MP",id);
				usleep(retardoMemoria);

				if(paginaBuscada->bitPresencia){
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger, "[%d][E] Se encontro en Tabla de Paginas y esta en memoria",id);
					log_info(activeLogger, "[%d][E] Realizando ESCRITURA en [Pag,Off,Bytes] = [%d,%d,%d]",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes);

					log_info(activeLogger, "[%d][E] Accediendo a MP",id);
					usleep(retardoMemoria);

					pthread_mutex_lock(&lock_accesoTabla);
					paginaBuscada->bitModificacion = 1;
					pthread_mutex_unlock(&lock_accesoTabla);

					pthread_mutex_lock(&lock_accesoMemoria);
					memcpy(memoria+(paginaBuscada->marcoUtilizado*config.tamanio_marco)+pedido.offset, buffer, pedido.cantBytes);
					pthread_mutex_unlock(&lock_accesoMemoria);

					ponerBitModif1(pedido.pid,pedido.paginaRequerida);

					agregarATlb(paginaBuscada,pedido.pid);

					log_info(activeLogger, "[%d][E] Agregado a TLB [Pagina,Marco] = [%d,%d]",id,pedido.paginaRequerida,paginaBuscada->marcoUtilizado);

					return "";
				}
	// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y RECIEN AHI LA DEVUELVOl, SI NO HAY PAGINAS DISPONIBLES: ALGORITMO DE SUSTITUCION DE PAGINAS
				else{
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger, "[%d][E] Se encontro en Tabla de Paginas pero NO ESTA EN MEMORIA. Buscando en SWAP: [Pag]=[%d]",id,pedido.paginaRequerida);
					log_info(activeLogger, "[%d][E] -----------SWAP----------",id);

					int pudo = buscarEnSwap(pedido,cliente);
					if(pudo){
						agregarATlb(paginaBuscada,pedido.pid);
						log_info(activeLogger, "[%d][E] Se encontro en SWAP [Pag]=[%d] y se agrego a memoria. Realizando pedido de LECTURA nuevamente",id,pedido.paginaRequerida);
						log_info(activeLogger, "[%d][E] -------------------------",id);
						almacenarBytesEnUnaPagina(pedido,buffer,cliente);
					}
					else{
						log_info(activeLogger, "[%d][E] No se encontro en SWAP [Pag]=[%d]",id,pedido.paginaRequerida);
					}
				}
			}// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
					enviarHeader(cliente.socket,HeaderNoExistePagina);
			}
			// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		}
		else{
				enviarHeader(cliente.socket,HeaderNoExisteTablaDePag);
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

void imprimirRegionMemoriaStack(char* region, int size){
	int i=0;
	printf("(REGION STACK):");
	int valor;
	while(i<size){
		valor = char4ToInt(region+i);
		printf("%d",valor);
		i=i+4;
	}
	printf("\n");
}

void imprimirRegionMemoriaCodigo(char* region, int size){
	int i;
	for(i=0;i<size;i++){
//			putchar(region[i]);
			printf("%c",region[i]);
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

				if(j<=cantidadPaginasDeTabla-paginas_stack){
					imprimirRegionMemoriaCodigo(contenido, config.tamanio_marco);
				}else{
					imprimirRegionMemoriaStack(contenido, config.tamanio_marco);
				}
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

void devolverMemoriaDePid(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* unaTabla = malloc(sizeof(tabla_t));

	unaTabla = buscarTabla(pid);
	int cantidadPaginasDeTabla = list_size((t_list*)unaTabla->listaPaginas);
	int i;

	for(i=0;i<cantidadPaginasDeTabla;i++){
		tablaPagina_t* unaPagina;
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

			printf("%s \n",contenido);
			if(i<=cantidadPaginasDeTabla-paginas_stack){
				imprimirRegionMemoriaCodigo(contenido, config.tamanio_marco);
			}else{
				imprimirRegionMemoriaStack(contenido, config.tamanio_marco);
			}

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

HILO recibirComandos(){
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
	return 0;
}
// FIN 2

// 3. Inicializar estructura de UMC
void crearMemoriaYTlbYTablaPaginas(){

	//Creo memoria y la relleno
	tamanioMemoria = config.cantidad_marcos * config.tamanio_marco;
	memoria = malloc(tamanioMemoria);
	memset(memoria,'\0',tamanioMemoria);
	log_info(activeLogger,"Creada la memoria\n");

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

	paginas_stack = 2;

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

int reservarPagina(int cantPaginasPedidas, int pid) {

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

char* getScript(t_cliente clienteNucleo) {
	log_debug(bgLogger, "Recibiendo archivo de nucleo %d...");
	char scriptSize;
	char* script;
	int size;
	read(clienteNucleo.socket, &scriptSize, sizeof(int));
	size = char4ToInt(&scriptSize);
	printf("%d",size);
	log_debug(bgLogger, "Nucleo envió un archivo de tamaño: %d", size);
	printf("Size:%d\n", size);
	script = malloc(sizeof(char) * size);
	read(clienteNucleo.socket, script, size);
	log_info(activeLogger, "Script de nucleo %d recibido:\n%s", clienteNucleo.indice, script);
	return script;
}

void pedidoLectura(t_cliente cliente){

	t_pedido* pedidoCpu = malloc(sizeof(t_pedido));
	char* pedidoSerializado = malloc(sizeof(t_pedido));
	int id=0;
//	MUTEXCLIENTES(id = clientes[cliente.indice].pid)

	read(cliente.socket, pedidoSerializado, sizeof(t_pedido));

	imprimir_serializacion(pedidoSerializado,12);
	deserializar_pedido(pedidoCpu, pedidoSerializado);

	pedidoLectura_t pedidoLectura;
	pedidoLectura.pid = id;
	pedidoLectura.paginaRequerida = pedidoCpu->pagina;
	pedidoLectura.offset = pedidoCpu->offset;
	pedidoLectura.cantBytes = pedidoCpu->size;


	log_info(activeLogger, "[%d] Realizando lectura de [Pag,Off,Bytes] = [%d,%d,%d]",id,pedidoLectura.paginaRequerida,pedidoLectura.offset,pedidoLectura.cantBytes);

	if(!existePaginaBuscadaEnTabla(pedidoCpu->pagina,buscarTabla(id))){
		send_w(cliente.socket, intToChar4(0),sizeof(int));
		return;
	}else{
		send_w(cliente.socket, intToChar4(1),sizeof(int));
	}

	char* contenidoAEnviar =  devolverPedidoPagina(pedidoLectura,cliente);

	if(pedidoLectura.paginaRequerida<=cantPaginasDePid(pedidoLectura.pid)-paginas_stack){
		imprimirRegionMemoriaCodigo(contenidoAEnviar, pedidoLectura.cantBytes);
	}else{
		imprimirRegionMemoriaStack(contenidoAEnviar, pedidoLectura.cantBytes);
	}

	send_w(cliente.socket, contenidoAEnviar,pedidoCpu->size);
}

void headerEscribirPagina(t_cliente cliente){
	t_pedido* pedidoCpuEscritura = malloc(sizeof(t_pedido));
	char* pedidoSerializadoEscritura = malloc(sizeof(t_pedido));
	int id =0;
//	MUTEXCLIENTES(id=clientes[cliente.indice].pid);

	read(cliente.socket, pedidoSerializadoEscritura, sizeof(t_pedido));
	deserializar_pedido(pedidoCpuEscritura,pedidoSerializadoEscritura);

	char* buffer = malloc(pedidoCpuEscritura->size);

	if(!existePaginaBuscadaEnTabla(pedidoCpuEscritura->pagina,buscarTabla(id))){
		send_w(cliente.socket, intToChar4(0),sizeof(int));
		return;
	}else{
		send_w(cliente.socket, intToChar4(1),sizeof(int));
	}

	read(cliente.socket, buffer, sizeof(int));

	pedidoLectura_t pedido;
	pedido.pid = id;
	pedido.paginaRequerida = pedidoCpuEscritura->pagina;
	pedido.offset = pedidoCpuEscritura->offset;
	pedido.cantBytes = pedidoCpuEscritura->size;

	log_info(activeLogger, "[%d] Realizando escritura de: [%d]  en [Pag,Off,Bytes] = [%d,%d,%d]",id,buffer,pedido.paginaRequerida,pedido.offset,pedido.cantBytes);


	almacenarBytesEnUnaPagina(pedido,buffer,cliente);
	send_w(cliente.socket, intToChar4(1),sizeof(int));
}

// FIN 4
void procesarHeader(t_cliente cliente, char* header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	// Ahora procesarHeader recibe un t_cliente!!!!! pero solo sirve para consultar los datos que no van a cambiar del cliente
	// tales como el socket, el indice en el vector. Para otras cosas consultar clientes[cliente.indice] con mutex!
	log_info(activeLogger, "Llego un mensaje con header %d",
			charToInt(header));
	int idLog=0;
//	MUTEXCLIENTES(idLog = clientes[cliente.identidad].pid;)

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "[%d] Header de Error",idLog);
		break;

	case HeaderHandshake:
		atenderHandshake(cliente);
		break;

	case HeaderTamanioPagina:
		log_info(activeLogger, "[%d] Pedido tamanio paginas",idLog);
		send_w(cliente.socket,intToChar4(config.tamanio_marco),sizeof(int));
		break;

	case HeaderPedirValorVariable:  //PARA NUCLEO
		log_info(activeLogger, "[%d] Pedido de lectura de Nucleo",idLog);
		pedidoLectura(cliente);
		break;

	case HeaderSolicitudSentencia: //PARA CPU
		log_info(activeLogger, "[%d] Pedido lectura de CPU",idLog);
		pedidoLectura(cliente);
		break;

	case HeaderScript: //Inicializar programa  // OK
		log_info(activeLogger, "[%d] Pedido Inicializar un programa de Nucleo",idLog);
		operacionScript(cliente);
		break;

	case HeaderAsignarValor: // CPU
		log_info(activeLogger, "[%d] Pedido escritura de CPU",idLog);
		headerEscribirPagina(cliente);
		break;

	case HeaderLiberarRecursosPagina:
		log_info(activeLogger, "[%d] Pedido de liberar recursos",idLog);
		char* pidALiberar = malloc(sizeof(int));
		read(cliente.socket , pidALiberar, sizeof(int));
		finalizarPrograma(atoi(pidALiberar));
		break;

	default:
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		log_warning(activeLogger,"Se quitará al cliente %d.",cliente.indice);
		quitarCliente(cliente.indice);
		break;
	}
}

void operacionScript(t_cliente cliente) {
	char* pidScript = NULL; // FALTAN MALLOCS ACA
	char* cantidadDePaginasScript = NULL;
	char* tamanioCodigoScript = NULL;
	char* codigoScript = NULL;
	//			read(cliente.socket , cantidadDePaginasScript, 4);
	read(cliente.socket, tamanioCodigoScript, 4);
	read(cliente.socket, pidScript, 4);
	read(cliente.socket, codigoScript, atoi(tamanioCodigoScript));

	if (inicializarPrograma(atoi(pidScript), codigoScript,
			char4ToInt(tamanioCodigoScript))) {
		reservarPagina(atoi(cantidadDePaginasScript), atoi(pidScript));
		reservarPagina(paginas_stack, atoi(pidScript));
		send_w(cliente.socket, "1", sizeof(int));
	} else {
		send_w(cliente.socket, "0", sizeof(int));
	}
}

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

// HILO PRINCIPAL: main()
// FUNCIONES: procesar las nuevas conexiones y crearles un hilo propio
int main(void) { //campo pid a tabla paginas, y en vez de list_get buscarRecursivo


	crearLogs("Umc", "Proceso", 0);
	dump = log_create("dump", "UMC", false, LOG_LEVEL_INFO);
	log_info(activeLogger,"Soy umc de process ID %d.\n", getpid());
	cargarCFG();
	iniciarAtrrYMutexs(1, &mutexClientes);

	listaTablasPaginas = list_create();
	log_info(activeLogger,"Creada la tabla de paginas");
	crearMemoriaYTlbYTablaPaginas();

//	log_info(activeLogger, "Conectando a SWAP ...");
//	conectarASwap();
//
//	crearHilo(&hiloRecibirComandos,(HILO)recibirComandos);
//
//	configurarServidorExtendido(&socketNucleo, &direccionNucleo,
//			config.puerto_umc_nucleo, &tamanioDireccionNucleo, &activadoNucleo);
//
//	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
//			&tamanioDireccionCPU, &activadoCPU);
//
//	inicializarClientes();
//	log_info(activeLogger, "Esperando conexiones CPU/NUCLEO...");
//
//	while (1) {
//		FD_ZERO(&socketsParaLectura);
//		FD_SET(socketNucleo, &socketsParaLectura);
//		FD_SET(socketCPU, &socketsParaLectura);
//
//		mayorDescriptor = (socketNucleo > socketCPU) ? socketNucleo : socketCPU;
//
//		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, NULL);
//
//		int cliente;
//		if (tieneLectura(socketCPU)) {
//			log_info(activeLogger, "Se conecto una nueva CPU");
//			if ((cliente = procesarNuevasConexionesExtendido(&socketCPU)) >= 0)
//				crearHiloConParametro(&clientes[cliente].hilo,
//						(HILO) hiloDedicado, (void*) cliente);
//		}
//		if (tieneLectura(socketNucleo)) {
//			log_info(activeLogger, "Se conecto Nucleo");
//			if ((cliente = procesarNuevasConexionesExtendido(&socketNucleo))
//					>= 0)
//				crearHiloConParametro(&clientes[cliente].hilo,
//						(HILO) hiloDedicado, (void*) cliente);
//		}
//	}

	test2();

	finalizar();
	return 0;
}






















// HILO HIJO: cpu()
// FUNCION: Atender los headers de las cpus
HILO hiloDedicado(int indice) {
	log_info(activeLogger, "Se creó un hilo dedicado");
	t_cliente clienteLocal; // Generamos una copia del cliente, no sirve para datos actualizables como el pid, solo para permanentes como socket e indice, para los demas campos consultar con mutex el vector de clientes
	MUTEXCLIENTES(clienteLocal = clientes[indice]);
	printf("SOCKET:%d\n",clienteLocal.socket);
	printf("INDICE:%d\n",clienteLocal.indice);
	char* header = malloc(1);
	while (recv(clienteLocal.socket, header, 1, MSG_WAITALL)>=0){
		procesarHeader(clienteLocal, header);
	}
	free(header);
	log_info(activeLogger, "Hasta aqui llego el hilo");
	return 0;
}

void atenderHandshake(t_cliente cliente){
	log_info(activeLogger, "Llego un handshake");
	char* handshake = malloc(1);
	read(cliente.socket,handshake,1);

	if ((charToInt(handshake) == SOYCPU) || (charToInt(handshake) == SOYNUCLEO)) {
		log_info(activeLogger,
				"Es un cliente apropiado! Respondiendo handshake");
		send(cliente.socket, intToChar(SOYUMC), 1, 0);
		if (charToInt(handshake) == SOYNUCLEO){
			// Acciones especificas de nucleo despues del handshake
			log_info(activeLogger,
					"Enviando tamaño de pagina a Nucleo");
			char* serialTamanio = intToChar4(config.tamanio_marco);
			send_w(cliente.socket,serialTamanio,sizeof(int));
			free(serialTamanio);
		}
		else if (charToInt(handshake) == SOYCPU){
			// Acciones especificas de cpu despues del handshake
			MUTEXSWAP(ejemploSWAP(cliente));
			MUTEXCLIENTES(quitarCliente(cliente.indice))
		}

	} else {
		log_error(activeLogger,
				"No es un cliente apropiado! rechazada la conexion");
		log_warning(activeLogger,"Se quitará al cliente %d.",cliente.indice);
		MUTEXCLIENTES(quitarCliente(cliente.indice))
	}
	free(handshake);
}


void mostrarTlb(){
	if(config.entradas_tlb){
		int i;
		printf("\n -- TLB -- \n");
		for(i=0;i<config.entradas_tlb;i++){
			tlb_t unaEntrada;
			unaEntrada = tlb[i];

			printf("Pos: %d Pid: %d Pag: %d Marco: %d Contador: %d \n",i,unaEntrada.pid,unaEntrada.pagina,unaEntrada.marcoUtilizado,unaEntrada.contadorTiempo);

		}
	}
}

void test2(){
	t_cliente clienteTest;

	reservarPagina(3,0); // 3 para el codigo
	reservarPagina(paginas_stack,0); //Para datos

		pedidoLectura_t pedido3;
			pedido3.pid=0;
			pedido3.paginaRequerida=1; //Pagina de codigo. Char*
			pedido3.offset=5;
			pedido3.cantBytes=2;

		devolverPedidoPagina(pedido3,clienteTest);

		mostrarTlb();

		pedidoLectura_t pedido4;
			pedido4.pid=0;
			pedido4.paginaRequerida=3; //Pagina de datos. Int, leo de a 4 bytes
			pedido4.offset=3;
			pedido4.cantBytes=4;

		devolverPedidoPagina(pedido4,clienteTest);

	devolverTodaLaMemoria();

	mostrarTlb();
}

// 5.Server de los cpu y de nucleo

void servidorCPUyNucleoExtendido(){

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
		log_info(activeLogger,"Umc recibio handshake de Swap.");
}

void conectarASwap(){
	direccion = crearDireccionParaCliente(config.puerto_swap,config.ip_swap);  //CAMBIAR ESTO DE IP
	swapServer = socket_w();
	connect_w(swapServer, &direccion);
	log_info(activeLogger,"Conexion a swap correcta :).");
	handshakearASwap();
	log_info(activeLogger,"Handshake finalizado exitosamente.");
}

void ejemploSWAP(t_cliente cliente){
	char* serialPID = intToChar4(cliente.indice);
		char* serialCantidadPaginas = intToChar4(5);
		char* serialPagina = intToChar4(2);
		char* contenidoPagina = malloc(config.tamanio_marco);
		memcpy(contenidoPagina,"abcdefg",7);
		bzero(contenidoPagina+7,config.tamanio_marco-7);

		// INICIAR PROCESO
		enviarHeader(swapServer,HeaderOperacionIniciarProceso);
		send_w(swapServer,serialPID,sizeof(int));
		send_w(swapServer,serialCantidadPaginas,sizeof(int));
		char* header = recv_waitall_ws(swapServer,1);
		if (charToInt(header)==HeaderProcesoAgregado)
			printf("Contesto el proceso Agregado\n");
		else
		if (charToInt(header)==HeaderNoHayEspacio){
			printf("No hay espacio\n");
			return;}
		else{
			printf("Llego mierda %d\n",(int)header);return;}

		// ESCRIBIR PAGINA
		enviarHeader(swapServer,HeaderOperacionEscritura);
		send_w(swapServer,serialPID,sizeof(int));
		send_w(swapServer,serialPagina,sizeof(int));
		send_w(swapServer,contenidoPagina,config.tamanio_marco);
		header = recv_waitall_ws(swapServer,1);
			if (charToInt(header)==HeaderEscrituraCorrecta)
				log_info(activeLogger,"Escritura correcta");
			else if (charToInt(header)==HeaderEscrituraErronea)
				log_warning(activeLogger,"Escritura erronea");
			else log_error(activeLogger,"Llego mierda al escribir");


		// LEER PAGINA
		enviarHeader(swapServer,HeaderOperacionLectura);
		char* contenidoPagina2 = malloc(config.tamanio_marco+1);
		send_w(swapServer,serialPID,sizeof(int));
		send_w(swapServer,serialPagina,sizeof(int));

		header = recv_waitall_ws(swapServer,1);
		if (charToInt(header)==HeaderOperacionLectura)
			log_info(activeLogger,"Contesto con la pagina");
		else if (charToInt(header)==HeaderProcesoNoEncontrado)
			log_warning(activeLogger,"No la encontró");
		else log_error(activeLogger,"Llego mierda al leer");

		contenidoPagina2 = recv_waitall_ws(swapServer,config.tamanio_marco);
		contenidoPagina2[config.tamanio_marco]='\0';
		log_info(activeLogger,"Llego el msg:%s",contenidoPagina2);
	//	log_info(activeLogger,"Llego el contenido y es igual:%d\n",strcmp(contenidoPagina,contenidoPagina2)==0);

		// FINALIZAR PROCESO
		enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
		send_w(swapServer,serialPID,sizeof(int));

		header = recv_waitall_ws(swapServer,1);
		if (charToInt(header)==HeaderProcesoEliminado)
			log_info(activeLogger,"Se elimino bien");
		else if (charToInt(header)==HeaderProcesoNoEncontrado)
			log_warning(activeLogger,"Se elimino mal");
		else log_error(activeLogger,"Llego mierda al leer");

		free(serialPID);
		free(header);
		free(serialCantidadPaginas);
		free(serialPagina);
		free(contenidoPagina);
		free(contenidoPagina2);
}

// FIN 6
