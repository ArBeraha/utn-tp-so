/*
 * umcNueva.c
 *
 *  Created on: 11/6/2016
 *      Author: utnso
 */
#include "umc.h"

// HILO PRINCIPAL: main()
// FUNCIONES: procesar las nuevas conexiones y crearles un hilo propio
HILO main2() {
	crearLogs("Umc","Proceso",0);
	log_info(activeLogger, "Iniciando umcNueva");
	cargarCFG();
	iniciarAtrrYMutexs(1,&mutexClientes);
	log_info(activeLogger, "Conectando a SWAP ...");
	conectarASwap();

	configurarServidorExtendido(&socketNucleo, &direccionNucleo, config.puerto_umc_nucleo,
					&tamanioDireccionNucleo, &activadoNucleo);

	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
			&tamanioDireccionCPU, &activadoCPU);

	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones ...");

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);
		FD_SET(socketNucleo, &socketsParaLectura);

		mayorDescriptor = (socketNucleo > socketCPU) ? socketNucleo : socketCPU;

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, NULL);

		int cliente;
		if (tieneLectura(socketCPU))  {
			log_info(activeLogger, "Se conecto una nueva CPU");
			cliente = procesarNuevasConexionesExtendido(&socketCPU);
			crearHiloConParametro(&clientes[cliente].hilo,(HILO)cpu,(void*)cliente);
		}
		if (tieneLectura(socketNucleo)) {
			log_info(activeLogger, "Se conecto Nucleo");
			cliente = procesarNuevasConexionesExtendido(&socketNucleo);
			crearHiloConParametro(&clientes[cliente].hilo,(HILO)nucleo,(void*)cliente);
		}
	}
}


void llamarSwap(t_cliente* cliente){
	char* serialPID = intToChar4(cliente->indice);
	char* serialCantidadPaginas = intToChar4(5);
	char* serialPagina = intToChar4(2);
	char* contenidoPagina = malloc(config.tamanio_marco);
	contenidoPagina = "abcdefg";

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
//	enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
//	send_w(swapServer,serialPID,sizeof(int));
//
//	header = recv_waitall_ws(swapServer,1);
//	if (charToInt(header)==HeaderProcesoEliminado)
//		log_info(activeLogger,"Se elimino bien");
//	else if (charToInt(header)==HeaderProcesoNoEncontrado)
//		log_warning(activeLogger,"Se elimino mal");
//	else log_error(activeLogger,"Llego mierda al leer");

	free(serialPID);
	free(header);
	free(serialCantidadPaginas);
	free(serialPagina);
	//free(contenidoPagina); ROMPE
	free(contenidoPagina2);
}


// HILO HIJO: cpu()
// FUNCION: Atender los headers de las cpus
HILO cpu(int indice) {
	log_info(activeLogger, "Se creó un hilo dedicado para la CPU");
	t_cliente* cliente;
	MUTEXCLIENTES(cliente = &clientes[indice]);
	printf("SOCKET:%d\n",cliente->socket);
	printf("INDICE:%d\n",cliente->indice);
	char* header = malloc(1);
	int bytes;
	MUTEXCLIENTES(bytes = recv(cliente->socket, header, 1, MSG_WAITALL));
	do {
		procesarHeader2(cliente, header);
		MUTEXCLIENTES(bytes = recv(cliente->socket, header, 1, MSG_WAITALL))
	} while (bytes>0);
	free(header);
	log_info(activeLogger, "Hasta aqui llego el hilo de cpu");
	return 0;
}
void procesarHeader2(t_cliente* cliente, char* header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	// TRATAR cliente CON MUTEX SIEMPRE ya que es un puntero al vector compartido
	log_info(activeLogger, "Llego un mensaje con header %d",
			charToInt(header));

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error");
		MUTEXCLIENTES(cliente->atentido=false; quitarCliente(cliente->indice);)
		break;

	case HeaderHandshake:
		atenderHandshake(cliente);
//		MUTEXSWAP(llamarSwap(cliente);)
//		quitarCliente(cliente->indice);
		break;

	default:
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		MUTEXCLIENTES(
				log_warning(activeLogger,"Se quitará al cliente %d.",cliente->indice); quitarCliente(cliente->indice);)
		break;
	}
}

void atenderHandshake(t_cliente* cliente){
	log_info(activeLogger, "Llego un handshake");
	char* handshake = malloc(1);
	MUTEXCLIENTES(read(cliente->socket,handshake,1);)

	if ((charToInt(handshake) == SOYCPU) || (charToInt(handshake) == SOYNUCLEO)) {
		log_info(activeLogger,
				"Es un cliente apropiado! Respondiendo handshake");
		MUTEXCLIENTES(
				cliente->identidad = charToInt(handshake); send(cliente->socket, intToChar(SOYUMC), 1, 0);)
		if (charToInt(handshake) == SOYNUCLEO){
			char* serialTamanio = intToChar4(config.tamanio_marco);
			send_w(cliente->socket,serialTamanio,sizeof(int));
			free(serialTamanio);
		}

	} else {
		log_error(activeLogger,
				"No es un cliente apropiado! rechazada la conexion");
		MUTEXCLIENTES(
				log_warning(activeLogger,"Se quitará al cliente %d.",cliente->indice); quitarCliente(cliente->indice);)
	}
	free(handshake);
}


HILO nucleo(int indice){
	log_info(activeLogger, "Se creó un hilo dedicado para nucleo");
	t_cliente* cliente;
	MUTEXCLIENTES(cliente = &clientes[indice]);
	printf("SOCKET:%d\n",cliente->socket);
	printf("INDICE:%d\n",cliente->indice);
	char* header = malloc(1);
	int bytes;
	MUTEXCLIENTES(bytes = recv(cliente->socket, header, 1, MSG_WAITALL));
	do {
		procesarHeader2(cliente, header);
		MUTEXCLIENTES(bytes = recv(cliente->socket, header, 1, MSG_WAITALL))
	} while (bytes>0);
	free(header);
	log_info(activeLogger, "Hasta aqui llego el hilo de nucleo");
	return 0;
}
