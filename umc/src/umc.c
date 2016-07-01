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
	config.mostrar_tlb = config_get_int_value(configUmc, "MOSTRAR_TLB");
	config.mostrar_paginas = config_get_int_value(configUmc, "MOSTRAR_PAGINAS");
	config.mostrar_MemoriaAlFinalizar = config_get_int_value(configUmc, "MOSTRAR_MEMALFINALIZ");

}

char* devolverPedidoPagina(pedidoLectura_t pedido, t_cliente cliente){
//	char* resultado = malloc(config.tamanio_marco);
	char* resultado = devolverBytes(pedido,cliente);
	if(strcmp(resultado,"RELLAMAR")==0){
		log_info(activeLogger, "[%d][L] Re-inicializando pedido de lectura por Page Fault",pedido.pid);
		return devolverBytes(pedido,cliente);
	}else{
		return resultado;
	}
}

char* almacenarBytesEnUnaPagina(pedidoLectura_t pedido, char* buffer,t_cliente cliente){
//	char* resultado = malloc(config.tamanio_marco);
	char* resultado = almacenarBytes(pedido,buffer,cliente);
	if(strcmp(resultado,"RELLAMAR")==0){ //SIGNIFICA QUE SON DISTINTOS
		log_info(activeLogger, "[%d][E] Re-inicializando pedido de escritura por Page Fault",pedido.pid,pedido.paginaRequerida);
		return almacenarBytes(pedido,buffer,cliente);
	}else{
		return resultado;
	}
}

char* devolverBytes(pedidoLectura_t pedido, t_cliente cliente){

//SI ESTA EN TLB DEVUELVO
	int id = 0;
	id = clientes[cliente.indice].pid;

	if(estaEnTlb(pedido) && config.entradas_tlb){

		int pos = buscarEnTlb(pedido);

		char* contenido = malloc(pedido.cantBytes);

		log_info(activeLogger, "[%d][L] TLB HIT. Se encontro en TLB para LECTURA [Pag,Off,Bytes] = [%d,%d,%d] en MARCO: %d",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes,tlb[pos].marcoUtilizado);

		log_info(activeLogger, "[%d][L] Accediendo a MP",id);
		usleep(retardoMemoria);

		pthread_mutex_lock(&lock_accesoMemoria);
		memcpy(contenido,memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);

		pthread_mutex_unlock(&lock_accesoMemoria);

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

				log_info(activeLogger, "[%d][L] TLB MISS. Accediendo a MP",id);
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
					if(config.mostrar_tlb)mostrarTlb();
//					log_info(activeLogger, "[%d][L] Agregado a TLB [Pagina,Marco] = [%d,%d]",id,pedido.paginaRequerida,paginaBuscada->marcoUtilizado);

					return contenido;

				}
// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y VUELVO A LLAMAR A FUNCION
				else{
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger, "[%d][L] Se encontro en Tabla de Paginas pero NO ESTA EN MEMORIA. Buscando en SWAP: [Pag]=[%d]",id,pedido.paginaRequerida);
					log_info(activeLogger, "[%d][L]-------------SWAP-----------",id);

					buscarEnSwap(pedido,cliente);

					log_info(activeLogger, "[%d][L] -------------------------------",id);

//					agregarATlb(paginaBuscada,pedido.pid);
					return "RELLAMAR";
//
//					if(pudo){
//						log_info(activeLogger, "[%d][L] Se encontro en SWAP [Pag]=[%d] y se agrego a memoria. Realizando pedido de LECTURA nuevamente",id,pedido.paginaRequerida);
//						log_info(activeLogger, "[%d][L]---------------------------",id);
//						devolverPedidoPagina(pedido, cliente);
//					}
//					else{
//						log_info(activeLogger, "[%d][L] NO se encontro en SWAP [Pag]=[%d]",id,pedido.paginaRequerida);
//						return "Error busqueda en swap";
//					}
				}
			}
// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
				printf("ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR 1!!!!");
				enviarHeader(cliente.socket,HeaderNoExistePagina);
			}
		}
// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		else{
			printf("ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR 2!!!!");
			enviarHeader(cliente.socket,HeaderNoExisteTablaDePag);
		}
	}
	return NULL;
}

char* almacenarBytes(pedidoLectura_t pedido, char* buffer,t_cliente cliente){

	int id =0;
	id=clientes[cliente.indice].pid;

	if(estaEnTlb(pedido) && config.entradas_tlb){

		int pos = buscarEnTlb(pedido);

		log_info(activeLogger, "[%d][E] TLB HIT. Se encontro en TLB para ESCRITURA [Pag,Off,Bytes] = [%d,%d,%d] en MARCO: %d",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes,tlb[pos].marcoUtilizado);

		log_info(activeLogger, "[%d][E] Accediendo a MP",id,pedido.paginaRequerida,pedido.offset,pedido.cantBytes,tlb[pos].marcoUtilizado);
		usleep(retardoMemoria);

		pthread_mutex_lock(&lock_accesoMemoria);
		memcpy(memoria+(tlb[pos].marcoUtilizado*config.tamanio_marco)+pedido.offset, buffer, pedido.cantBytes);
		pthread_mutex_unlock(&lock_accesoMemoria);

		ponerBitModif1(pedido.pid,pedido.paginaRequerida);

		log_info(activeLogger, "[%d][E] Se almaceno: ",id);
																			     //C C S
		if(pedido.paginaRequerida<cantPaginasDePid(pedido.pid)- paginas_stack){ // 0 1 2   1
			imprimirRegionMemoriaCodigo(memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset, pedido.cantBytes);
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

				log_info(activeLogger, "[%d][E] TLB MISS. Accediendo a MP",id);
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
					if(config.mostrar_tlb)mostrarTlb();

//					log_info(activeLogger, "[%d][E] Agregado a TLB [Pagina,Marco] = [%d,%d]",id,pedido.paginaRequerida,paginaBuscada->marcoUtilizado);

					return "1";
				}
	// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y LA CARGO EN MEMORIA Y TLB Y RECIEN AHI LA DEVUELVOl, SI NO HAY PAGINAS DISPONIBLES: ALGORITMO DE SUSTITUCION DE PAGINAS
				else{
					pthread_mutex_unlock(&lock_accesoTabla);
					log_info(activeLogger, "[%d][E] Se encontro en Tabla de Paginas pero NO ESTA EN MEMORIA. Buscando en SWAP: [Pag]=[%d]",id,pedido.paginaRequerida);
					log_info(activeLogger, "[%d][E] -----------SWAP----------",id);

//					agregarATlb(paginaBuscada,pedido.pid);
					buscarEnSwap(pedido,cliente);
					log_info(activeLogger, "[%d][E] -------------------------------",id);
					return "RELLAMAR";
//					if(pudo){
//						log_info(activeLogger, "[%d][E] Se encontro en SWAP [Pag]=[%d] y se agrego a memoria. Realizando pedido de ESCRITURA nuevamente",id,pedido.paginaRequerida);
//						almacenarBytesEnUnaPagina(pedido,buffer,cliente);
//					}
//					else{
//						log_info(activeLogger, "[%d][E] No se encontro en SWAP [Pag]=[%d]",id,pedido.paginaRequerida);
//					}
				}
			}// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
				printf("ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR 3!!!!");
				enviarHeader(cliente.socket,HeaderNoExistePagina);
			}
			// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		}
		else{
			printf("ERRORRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR 4!!!!");
			enviarHeader(cliente.socket,HeaderNoExisteTablaDePag);
		}
	}
	return "Error ifs";
}

int buscarEnSwap(pedidoLectura_t pedido, t_cliente cliente){
	char* serialPID = intToChar4(pedido.pid);

	char* serialPagina = intToChar4(pedido.paginaRequerida);
	char* contenidoPagina = malloc(config.tamanio_marco);

	pthread_mutex_lock(&lock_accesoSwap);

	enviarHeader(swapServer,HeaderOperacionLectura);

	send_w(swapServer,serialPID,sizeof(int));
	send_w(swapServer,serialPagina,sizeof(int));

	free(serialPID);
	free(serialPagina);

	pthread_mutex_unlock(&lock_accesoSwap);

	contenidoPagina = recv_waitall_ws(swapServer,config.tamanio_marco);

	log_info(activeLogger, "[%d][E] Agregando a memoria la pagina buscada en SWAP: [Pag]=[%d]",pedido.pid,pedido.paginaRequerida);
	agregarAMemoria(pedido,contenidoPagina,cliente);
	log_info(activeLogger, "[%d][E] Finalizo la carga en memoria de la pagina buscada en SWAP: [Pag]=[%d]",pedido.pid,pedido.paginaRequerida);

	return 1;
}

void agregarAMemoria(pedidoLectura_t pedido, char* contenido, t_cliente cliente){
	int id=0;
	id=clientes[cliente.indice].pid;

	if(cantPaginasEnMemoriaDePid(pedido.pid)>=config.marcos_x_proceso || cantidadMarcosLibres()==0){
		int posicionPaginaSacada=0;

		if(strcmp(config.algoritmo_paginas,"CLOCK")==0){
			posicionPaginaSacada=sacarConClock(pedido.pid);
		}else {
			if(strcmp(config.algoritmo_paginas,"CLOCK_MODIFICADO")==0){
				posicionPaginaSacada=sacarConModificado(pedido.pid);
			}
			else{
				log_error(activeLogger, "Error sintaxis algoritmo: CLOCK o CLOCK_MODIFICADO");
			}

		}
		log_info(activeLogger, "[%d] No hay marcos disponibles. [Pag] Sacando [%d]. Agregando [%d]. Con: %s",id,posicionPaginaSacada,pedido.paginaRequerida, config.algoritmo_paginas);

		pthread_mutex_lock(&lock_accesoTabla);
		tabla_t* tablaPaginaAReemplazar = buscarTabla(pedido.pid);
		tablaPagina_t* paginaASacarDeMemoria = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas, posicionPaginaSacada);
		pthread_mutex_unlock(&lock_accesoTabla);

		if(paginaASacarDeMemoria->bitModificacion){
			pthread_mutex_lock(&lock_accesoSwap);
			enviarASwap(pedido.pid,paginaASacarDeMemoria);
			pthread_mutex_unlock(&lock_accesoSwap);
			pthread_mutex_lock(&lock_accesoTabla);
			paginaASacarDeMemoria->bitModificacion=0;
			pthread_mutex_unlock(&lock_accesoTabla);
		}

		int marcoSacado = paginaASacarDeMemoria->marcoUtilizado;
		log_info(activeLogger, "[%d] Intercambiando contenido de Marco [%d]",id,marcoSacado);

		sacarDeMemoria(paginaASacarDeMemoria,id);

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

//		flushTlb();
		pedido.cantBytes=config.tamanio_marco;
		pedido.offset=0;

//		almacenarBytesEnUnaPagina(pedido,contenido,cliente);
		almacenarBytes(pedido,contenido,cliente);
	}
	else{
		pthread_mutex_lock(&lock_accesoTabla);
		tabla_t* tablaPaginaAAgregar = buscarTabla(pedido.pid);
		tablaPagina_t* paginaACargar = list_get((t_list*)tablaPaginaAAgregar->listaPaginas,pedido.paginaRequerida);

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

		char* contenido2 = malloc(config.tamanio_marco+1);
		memcpy(contenido2,contenido,config.tamanio_marco);
		contenido[config.tamanio_marco]='\0';

//		almacenarBytesEnUnaPagina(pedido, contenido, cliente);
		almacenarBytes(pedido, contenido, cliente);
	}

}

int inicializarPrograma(int idPrograma, char* contenido,int cantPaginas){

	char* serialPID = intToChar4(idPrograma);
	char* serialCantidadPaginasTotales = intToChar4(cantPaginas+paginas_stack);
	char* serialCantidadPaginas = intToChar4(cantPaginas);
	int i=0;

	pthread_mutex_lock(&lock_accesoSwap);
	enviarHeader(swapServer,HeaderOperacionIniciarProceso);
	send_w(swapServer,serialPID,sizeof(int));
	send_w(swapServer,serialCantidadPaginasTotales,sizeof(int));
	send_w(swapServer,serialCantidadPaginas,sizeof(int));


	free(serialPID);
	free(serialCantidadPaginasTotales);
	free(serialCantidadPaginas);

	char* fraccionCodigo = malloc(config.tamanio_marco);
	for(i=0;i<cantPaginas;i++){
		memcpy(fraccionCodigo,contenido+(i*config.tamanio_marco),config.tamanio_marco);
		send_w(swapServer,fraccionCodigo,config.tamanio_marco);
	}

	char* header = recv_waitall_ws(swapServer,1);
	pthread_mutex_unlock(&lock_accesoSwap);

	if (charToInt(header)==HeaderProcesoAgregado){
		log_info(activeLogger, "[S] Swap almaceno el script correctamente");
		return 1;
	}else{
		return 0;
	}

	free(header);
}

t_cliente* buscarClientePorPid(int pid){
	int i;
	for (i=0;i<getMaxClients();i++){
		if (clientes[i].pid==pid)
			return &clientes[i];
	}
	return NULL;
}

void finalizarPrograma(int idPrograma){
	pthread_mutex_lock(&lock_accesoSwap);
	enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
	char* serialIdPrograma = intToChar4(idPrograma);
	send_w(swapServer,serialIdPrograma,sizeof(int));
	pthread_mutex_unlock(&lock_accesoSwap);
	if (existePidEnListadeTablas(idPrograma)){
		sacarMarcosOcupados(idPrograma);
		flushTlbDePid(idPrograma);
		tabla_t* tabla = buscarTabla(idPrograma);
		list_destroy((t_list*)tabla->listaPaginas);
		list_remove(listaTablasPaginas,buscarPosicionTabla(idPrograma));
//		close(buscarClientePorPid(idPrograma)->socket);
	}
	log_info(activeLogger,"[%d] Se finalizo el proceso",idPrograma);
	free(serialIdPrograma);
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


void pedidoLectura(t_cliente cliente) {

	if(config.mostrar_paginas){ devolverTodasLasPaginas(); printf("\n");}

	t_pedido* pedidoCpu = malloc(sizeof(t_pedido));
	char* pedidoSerializado = malloc(sizeof(t_pedido));
	int id = 0;
	id = clientes[cliente.indice].pid;

	read(cliente.socket, pedidoSerializado, sizeof(t_pedido));

	if (estaConectado(cliente) && buscarTabla(id)!=NULL) {

		deserializar_pedido(pedidoCpu, pedidoSerializado);

		pedidoLectura_t pedidoLectura;
		pedidoLectura.pid = id;
		pedidoLectura.paginaRequerida = pedidoCpu->pagina;
		pedidoLectura.offset = pedidoCpu->offset;
		pedidoLectura.cantBytes = pedidoCpu->size;

		log_info(activeLogger,
				"[%d] Realizando lectura de [Pag,Off,Bytes] = [%d,%d,%d]", id,
				pedidoLectura.paginaRequerida, pedidoLectura.offset,
				pedidoLectura.cantBytes);

		if (!existePaginaBuscadaEnTabla(pedidoCpu->pagina, buscarTabla(id))) {
			char* serialRespuesta = intToChar4(0);
			if (estaConectado(cliente))
					send_w(cliente.socket, serialRespuesta, sizeof(int));
			free(serialRespuesta);
			return;
		}
		else {
			tabla_t* tabla = buscarTabla(id);
			tablaPagina_t* pagina = list_get((t_list*)tabla->listaPaginas,pedidoLectura.paginaRequerida);
			if(pagina->bitPresencia == 0 && cantidadMarcosLibres()==0 && cantPaginasEnMemoriaDePid(id)==0){
				char* serialRespuesta = intToChar4(2);
				if (estaConectado(cliente))
					send_w(cliente.socket, serialRespuesta, sizeof(int));
			free(serialRespuesta);
			finalizarPrograma(id);
			return;
		}else{
			char* serialRespuesta = intToChar4(1);
			if (estaConectado(cliente))
				send_w(cliente.socket, serialRespuesta, sizeof(int));
			free(serialRespuesta);
			}
		}

		char* contenido = devolverPedidoPagina(pedidoLectura, cliente);

		if (estaConectado(cliente)) {
			log_info(activeLogger,"[%d] Devolviendo lectura: ", id);

			if(pedidoLectura.paginaRequerida<cantPaginasDePid(pedidoLectura.pid)- paginas_stack){ // 0 1 2   1
				imprimirRegionMemoriaCodigo(contenido, pedidoLectura.cantBytes);
			}else{
				imprimirRegionMemoriaStack(contenido, pedidoLectura.cantBytes);
			}

			send_w(cliente.socket, contenido, pedidoLectura.cantBytes);
		} else{
			printf("Se interrumpió la lectura por desconexion\n");
			log_error(activeLogger, "Se interrumpió la lectura por desconexion");

		}
		free(pedidoSerializado);
		free(pedidoCpu);
		log_info(activeLogger,ANSI_COLOR_RED "[%d] Finalizo pedido de lectura" ANSI_COLOR_RESET,id);
	}
}


void headerEscribirPagina(t_cliente cliente){

	if(config.mostrar_paginas){ devolverTodasLasPaginas(); printf("\n");}

	t_pedido* pedidoCpuEscritura = malloc(sizeof(t_pedido));
	char* pedidoSerializadoEscritura = malloc(sizeof(t_pedido));
	int id =0;
	id=clientes[cliente.indice].pid;

	read(cliente.socket, pedidoSerializadoEscritura, sizeof(t_pedido));
	deserializar_pedido(pedidoCpuEscritura,pedidoSerializadoEscritura);

	char* buffer = malloc(pedidoCpuEscritura->size);

//	if(!existePaginaBuscadaEnTabla(pedidoCpuEscritura->pagina,buscarTabla(id))){
//		char* serialRespuesta = intToChar4(0);
//		send_w(cliente.socket, serialRespuesta,sizeof(int));
//		free(serialRespuesta);
//		return;
//	}else{
//		char* serialRespuesta = intToChar4(1);
//		send_w(cliente.socket, serialRespuesta,sizeof(int));
//		free(serialRespuesta);
//	}

	if (!existePaginaBuscadaEnTabla(pedidoCpuEscritura->pagina, buscarTabla(id))) {
		char* serialRespuesta = intToChar4(0);
		if (estaConectado(cliente))
			send_w(cliente.socket, serialRespuesta, sizeof(int));
		free(serialRespuesta);
		return;
	}
	else {
		tabla_t* tabla = buscarTabla(id);
		tablaPagina_t* pagina = list_get((t_list*)tabla->listaPaginas,pedidoCpuEscritura->pagina);
		if(pagina->bitPresencia == 0 && cantidadMarcosLibres()==0 && cantPaginasEnMemoriaDePid(id)==0){
			char* serialRespuesta = intToChar4(2);
			if (estaConectado(cliente))
				send_w(cliente.socket, serialRespuesta, sizeof(int));
			free(serialRespuesta);
			return;
	}else{
		char* serialRespuesta = intToChar4(1);
		if (estaConectado(cliente))
			send_w(cliente.socket, serialRespuesta, sizeof(int));
		free(serialRespuesta);
		}
	}


	read(cliente.socket, buffer, sizeof(int));

	pedidoLectura_t pedido;
	pedido.pid = id;
	pedido.paginaRequerida = pedidoCpuEscritura->pagina;
	pedido.offset = pedidoCpuEscritura->offset;
	pedido.cantBytes = pedidoCpuEscritura->size;

	log_info(activeLogger, "[%d] Realizando escritura de: [%d]  en [Pag,Off,Bytes] = [%d,%d,%d]",id,char4ToInt(buffer),pedido.paginaRequerida,pedido.offset,pedido.cantBytes);

	almacenarBytesEnUnaPagina(pedido,buffer,cliente);

	free(pedidoSerializadoEscritura);
	free(pedidoCpuEscritura);
	log_info(activeLogger, ANSI_COLOR_RED "[%d] Finalizo pedido de escritura" ANSI_COLOR_RESET,id);
}

void operacionScript(t_cliente cliente) {
	char* pidScript = malloc(sizeof(int));
	char* cantidadDePaginasScript = malloc(sizeof(int));
	char* tamanioCodigoScript = malloc(sizeof(int));

	read(cliente.socket, pidScript, 4);
	log_info(activeLogger, ANSI_COLOR_GREEN "Pedido de inicializacion de PID: %d " ANSI_COLOR_RESET,char4ToInt(pidScript));
	read(cliente.socket, cantidadDePaginasScript, 4);
	read(cliente.socket, tamanioCodigoScript, 4);
	char* codigoScript = malloc(char4ToInt(tamanioCodigoScript));

	read(cliente.socket, codigoScript, char4ToInt(tamanioCodigoScript));

	if (inicializarPrograma(char4ToInt(pidScript), codigoScript,char4ToInt(cantidadDePaginasScript))) {

		reservarPagina(char4ToInt(cantidadDePaginasScript), char4ToInt(pidScript));
		reservarPagina(paginas_stack, char4ToInt(pidScript));
		log_info(activeLogger,"[%d] Se inicializo el proceso",char4ToInt(pidScript));
		char* serialRespuesta = intToChar(1);
		send_w(cliente.socket, serialRespuesta,1);
		free(serialRespuesta);
	} else {
		char* serialRespuesta = intToChar(0);
		send_w(cliente.socket, serialRespuesta,1);
		free(serialRespuesta);
	}

	free(pidScript);
	free(cantidadDePaginasScript);
	free(tamanioCodigoScript);

	log_info(activeLogger, ANSI_COLOR_RED "Finalizo pedido de inicializacion de programa" ANSI_COLOR_RESET);
}

void procesarHeader(t_cliente cliente, char* header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	// Ahora procesarHeader recibe un t_cliente!!!!! pero solo sirve para consultar los datos que no van a cambiar del cliente
	// tales como el socket, el indice en el vector. Para otras cosas consultar clientes[cliente.indice] con mutex!
//	log_info(activeLogger, "Llego un mensaje con header %d",
//			charToInt(header));
	char* nuevoPid;
	int idLog=0;
	int viejoPid=0;
	idLog = clientes[cliente.indice].pid;

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "[%d] Header de Error",idLog);
		break;

	case HeaderHandshake:
		atenderHandshake(cliente);
		break;

	case HeaderTamanioPagina:
		log_info(activeLogger, ANSI_COLOR_GREEN "[%d] Pedido tamanio paginas" ANSI_COLOR_RESET ,idLog);
		char* serialTamanioMarco = intToChar4(config.tamanio_marco);
		send_w(cliente.socket,serialTamanioMarco,sizeof(int));
		free(serialTamanioMarco);
		break;

	case HeaderPedirValorVariable:  //PARA NUCLEO
		log_info(activeLogger, ANSI_COLOR_GREEN "[%d] Pedido de lectura de Nucleo" ANSI_COLOR_RESET ,idLog);
		pedidoLectura(cliente);
		break;

	case HeaderSolicitudSentencia: //PARA CPU
		log_info(activeLogger,ANSI_COLOR_GREEN "[%d] Pedido lectura de CPU"ANSI_COLOR_RESET,idLog);
		pedidoLectura(cliente);
		break;

	case HeaderScript: //Inicializar programa  // OK
		log_info(activeLogger, ANSI_COLOR_GREEN "[%d] Pedido Inicializar un programa de Nucleo" ANSI_COLOR_RESET ,idLog);
		operacionScript(cliente);
		break;

	case HeaderAsignarValor: // CPU
		log_info(activeLogger, ANSI_COLOR_GREEN "[%d] Pedido escritura de CPU" ANSI_COLOR_RESET ,idLog);
		headerEscribirPagina(cliente);
		break;

	case HeaderLiberarRecursosPagina:
		log_info(activeLogger, ANSI_COLOR_GREEN  "[%d] Pedido de liberar recursos" ANSI_COLOR_RESET ,idLog);
		finalizarPrograma(char4ToInt(recv_nowait_ws(cliente.socket,sizeof(int))));
		break;

	case HeaderTerminoProceso:
		log_info(activeLogger, ANSI_COLOR_GREEN  "[%d] Finalizo proceso de CPU, liberando estructuras" ANSI_COLOR_RESET ,idLog);

		if(config.mostrar_MemoriaAlFinalizar){
			printf("-------- ANTES -------");
			devolverTodasLasPaginas();
			devolverTodaLaMemoria();
		}
		finalizarPrograma(idLog);

		if(config.mostrar_MemoriaAlFinalizar){
			printf("-------- DESPUES -------");
			devolverTodasLasPaginas();
			devolverTodaLaMemoria();
		}

		break;

	case HeaderPID:
		viejoPid = clientes[cliente.indice].pid;
		nuevoPid = malloc(sizeof(int));
		read(cliente.socket, nuevoPid, sizeof(int));
		clientes[cliente.indice].pid=char4ToInt(nuevoPid);

		if(viejoPid!=char4ToInt(nuevoPid)){
			flushTlbDePid(viejoPid);
			log_info(activeLogger,"Se limpio la TLB del Pid: %d ",viejoPid);
		}

		log_info(activeLogger, ANSI_COLOR_GREEN  "[%d] Cambio PID viejo: %d por nuevo: %d " ANSI_COLOR_RESET ,idLog,viejoPid,char4ToInt(nuevoPid));
//		devolverTodaLaMemoria();
		break;

	case headerCPUTerminada:
		log_info(activeLogger, ANSI_COLOR_GREEN  "[%d] Quitando CPU como cliente: %d" ANSI_COLOR_RESET ,clientes[cliente.indice]);
		quitarCliente(cliente.indice);
		break;

	default:
		log_error(activeLogger,ANSI_COLOR_RED
				"Llego el header numero %d y no hay una acción definida para él." ANSI_COLOR_RESET,
				charToInt(header));
		log_warning(activeLogger,"Se quitará al cliente %d.",cliente.indice);
		quitarCliente(cliente.indice);
		break;
	}
}


// HILO PRINCIPAL: main()
// FUNCIONES: procesar las nuevas conexiones y crearles un hilo propio
int main(void) { //campo pid a tabla paginas, y en vez de list_get buscarRecursivo

	system("rm -rf Umc.log");
	system("rm -rf dump");

	crearLogs("Umc", "UMC", -1);
	dump = log_create("dump", "UMC", false, LOG_LEVEL_INFO);

	log_info(activeLogger,"Soy umc de process ID %d", getpid());
	cargarCFG();
	iniciarAtrrYMutexs(8, &mutexClientes,&mutexSwap,
	&lock_accesoMemoria,
	&lock_accesoTabla,
	&lock_accesoTlb,
	&lock_accesoMarcosOcupados,
	&lock_accesoUltimaPos,
	&lock_accesoLog);

	listaTablasPaginas = list_create();
	log_info(activeLogger,"Creada la tabla de paginas");
	crearMemoriaYTlbYTablaPaginas();

	log_info(activeLogger, "Conectando a SWAP ...");
	conectarASwap();

	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
			&tamanioDireccionCPU, &activadoCPU);

	configurarServidorExtendido(&socketNucleo, &direccionNucleo,
			config.puerto_umc_nucleo, &tamanioDireccionNucleo, &activadoNucleo);

	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones CPU/NUCLEO...");

	crearHilo(&hiloRecibirComandos,(HILO)recibirComandos);

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);
		FD_SET(socketNucleo, &socketsParaLectura);

		mayorDescriptor = (socketNucleo > socketCPU) ? socketNucleo : socketCPU;

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, NULL);

		int cliente;
		if (tieneLectura(socketCPU)) {
			log_info(activeLogger, "Se conecto una nueva CPU");
			if ((cliente = procesarNuevasConexionesExtendido(&socketCPU)) >= 0)
				crearHiloConParametro(&clientes[cliente].hilo,
						(HILO) hiloDedicado, (void*) cliente);
		}
		if (tieneLectura(socketNucleo)) {
			log_info(activeLogger, "Se conecto Nucleo");
			if ((cliente = procesarNuevasConexionesExtendido(&socketNucleo))
					>= 0)
				crearHiloConParametro(&clientes[cliente].hilo,
						(HILO) hiloDedicado, (void*) cliente);
		}
	}

//	test2();

	finalizar();
	return 0;
}


void crearMemoriaYTlbYTablaPaginas(){

	//Creo memoria y la relleno
	tamanioMemoria = config.cantidad_marcos * config.tamanio_marco;
	memoria = malloc(tamanioMemoria);
	memset(memoria,'\0',tamanioMemoria);
	log_info(activeLogger,"Creada la memoria");

	tlb = malloc(config.entradas_tlb * sizeof(tlb_t));
	inicializarTlb();
	log_info(activeLogger,"Creada la TLB y rellenada con ceros (0).");

	//Creo vector de marcos ocupados y lo relleno
	vectorMarcosOcupados = malloc(sizeof(int) * config.cantidad_marcos);
	log_info(activeLogger,"Creado el vector de marcos ocupados");

	memset(vectorMarcosOcupados,0,sizeof(int) * config.cantidad_marcos);


	vectorClientes = malloc(MAXCLIENTS * sizeof(int));
	memset(vectorClientes,-1, MAXCLIENTS * sizeof(int));

	retardoMemoria = config.retardo;

	listaUltimaPosicionSacada = list_create();

	pthread_attr_init(&detachedAttr);
	pthread_attr_setdetachstate(&detachedAttr, PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&lock_accesoMarcosOcupados, NULL);
	pthread_mutex_init(&lock_accesoLog, NULL);
	pthread_mutex_init(&lock_accesoMemoria, NULL);
	pthread_mutex_init(&lock_accesoTabla, NULL);
	pthread_mutex_init(&lock_accesoTlb, NULL);
	pthread_mutex_init(&lock_accesoUltimaPos, NULL);

}

void finalizar() {
	destruirLogs();
	log_destroy(dump);
	list_destroy(listaTablasPaginas);
	free(tlb);
	free(memoria);
	free(vectorClientes);
	free(vectorMarcosOcupados);
	pthread_mutex_destroy(&lock_accesoMarcosOcupados);
	pthread_mutex_destroy(&lock_accesoMemoria);
	pthread_mutex_destroy(&lock_accesoTabla);
	pthread_mutex_destroy(&lock_accesoTlb);
	pthread_mutex_destroy(&lock_accesoUltimaPos);
}


//void test2(){
//	t_cliente clienteTest;
//
//	reservarPagina(3,0); // 3 para el codigo
//	reservarPagina(paginas_stack,0); //Para datos
//
//		pedidoLectura_t pedido3;
//			pedido3.pid=0;
//			pedido3.paginaRequerida=1; //Pagina de codigo. Char*
//			pedido3.offset=5;
//			pedido3.cantBytes=2;
//
//		devolverPedidoPagina(pedido3,clienteTest);
//
//		mostrarTlb();
//
//		pedidoLectura_t pedido4;
//			pedido4.pid=0;
//			pedido4.paginaRequerida=3; //Pagina de datos. Int, leo de a 4 bytes
//			pedido4.offset=3;
//			pedido4.cantBytes=4;
//
//		devolverPedidoPagina(pedido4,clienteTest);
//
//	devolverTodaLaMemoria();
//
//	mostrarTlb();
//}

