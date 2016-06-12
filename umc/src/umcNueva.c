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

	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
			&tamanioDireccionCPU, &activadoCPU);
	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones ...");

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);

		mayorDescriptor = (socketNucleo > socketCPU) ? socketNucleo : socketCPU;

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, NULL);

		int cliente;
		if (tieneLectura(socketCPU)) {
			log_info(activeLogger, "Hay lectura");
			cliente = procesarNuevasConexionesExtendido(&socketCPU);
			crearHiloConParametro(&clientes[cliente].hilo,(HILO)cpu,(void*)cliente);
		}
	}
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
	int b=1;
	while(b>0) {
		MUTEXCLIENTES(b = recv(cliente->socket, header, 1, MSG_WAITALL))
		procesarHeaderCPU(cliente, header);
	}
	log_info(activeLogger, "Hasta aqui llego el hilo de cpu");
	return header;
}

void procesarHeaderCPU(t_cliente* cliente, char* header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	// TRATAR cliente CON MUTEX SIEMPRE ya que es un puntero al vector compartido
	log_info(activeLogger, "Llego un mensaje con header %d",
			charToInt(header));
	MUTEXCLIENTES(cliente->atentido=true);

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error");
		MUTEXCLIENTES(cliente->atentido=false; quitarCliente(cliente->indice);)
		break;

	case HeaderHandshake:
		atenderHandshake(cliente);
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

	if (charToInt(handshake) == SOYCPU) {
		log_info(activeLogger,
				"Es un cliente apropiado! Respondiendo handshake");
		MUTEXCLIENTES(
				cliente->identidad = charToInt(handshake); send(cliente->socket, intToChar(SOYUMC), 1, 0);)
	} else {
		log_error(activeLogger,
				"No es un cliente apropiado! rechazada la conexion");
		MUTEXCLIENTES(
				log_warning(activeLogger,"Se quitará al cliente %d.",cliente->indice); quitarCliente(cliente->indice);)
	}
	free(handshake);
}
