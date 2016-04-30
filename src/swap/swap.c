/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/process.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "../otros/handshake.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/header.h"
#include "../otros/log.h"

#define PUERTO_SWAP 8082
int cliente;

void procesarHeader(int cliente, char* header)
{
	char* payload;
	int payload_size;
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));

    	switch(charToInt(header))
    	{

    	case HeaderError:
    		log_error(activeLogger,"Header de Error.");
    		break;

    	case HeaderHandshake:
    		log_debug(bgLogger, "Llego un handshake");
    		payload_size = 1;;
    		payload = malloc(payload_size);
    		//read(socketCliente[cliente], payload, payload_size);
    		payload= recv_waitall_ws(cliente,payload_size);
    		log_debug(bgLogger, "Llego un mensaje con payload %d",
    				charToInt(payload));
    		if (charToInt(payload) == SOYUMC) {
    			log_debug(bgLogger,
    					"Es un cliente apropiado! Respondiendo handshake");
    			send(cliente, intToChar(SOYSWAP), 1, 0);
    		} else {
    			log_error(activeLogger,
    					"No es un cliente apropiado! rechazada la conexion");
    			log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
    			quitarCliente(cliente);
    		}
    		free(payload);
    		break;


    	default:
    		log_error(activeLogger,"Llego cualquier cosa.");
    		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
    		exit(EXIT_FAILURE);
    		break;
    	}
    }


int main()
{
	char *header;
    crearLogs("Swap","Swap");

    printf("1");
	configurarServidor(PUERTO_SWAP);
	printf("2");
	log_info(activeLogger,"Esperando conexiones");
	printf("3");
	procesarNuevasConexiones();
	cliente=socketCliente[0];
	printf("4");

	while (1){
		header=recv_waitall_ws(cliente,1);
		procesarHeader(cliente,header);
	}

	return 0;
}

