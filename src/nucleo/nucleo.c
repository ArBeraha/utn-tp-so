/*
 * kernel.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <commons/string.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"

#define PUERTO 8080

void procesarHeader(int cliente, char* header){
	// Segun el protocolo procesamos el header del mensaje recibido
	int payload_size;
	char* payload;

	switch(atoi(header)) {

	case HeaderError:
		printf("Header de Error\n");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		printf("Llego 1\n");
		payload_size=1;
		payload = malloc(payload_size);
		read(socketCliente[cliente] , payload, payload_size);
		if ((atoi(payload)==SOYCONSOLA) || (atoi(payload)==SOYCPU)){
			printf("Es un cliente apropiado! Respondiendo handshake\n");
			send(socketCliente[cliente], string_itoa(SOYNUCLEO), payload_size, 0);
		}
		else {
			printf("No es un cliente apropiado! rechazada la conexion\n");
			quitarCliente(cliente);
		}
		free(payload);
		break;

	case HeaderScript: /*A implementar*/ break;
	/* Agregar futuros casos */

	default: printf("Llego cualquier cosa\n"); break;
	}
}

int main(void) {

	int mayorDescriptor, i;
	struct timeval espera; 		// Periodo maximo de espera del select
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	char header[sizeof(header_t)];

	configurarServidor();
	inicializarClientes();
	puts("Esperando conexiones ...");

	while(1){
		mayorDescriptor = incorporarSockets();
		select( mayorDescriptor + 1 , &socketsParaLectura , NULL , NULL , &espera);

		if (tieneLectura(socketNuevasConexiones))
			procesarNuevasConexiones();

		for (i = 0; i < getMaxClients(); i++){
			if (tieneLectura(socketCliente[i]))	{
				if (read( socketCliente[i] , header, sizeof(header_t)) == 0)
					quitarCliente(i);
				else
					procesarHeader(i,header);
			}
		}
	}
	return EXIT_SUCCESS;
}
