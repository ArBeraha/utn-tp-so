/*
 * kernel.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include "../otros/handshake.h"
#include "../otros/sockets/cliente-servidor.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <commons/string.h>

#define MAXCLIENTS 30
#define PUERTO 8080
#define HEADER_SIZE 1

struct sockaddr_in direccion;
int socketNuevasConexiones, socketCliente[MAXCLIENTS], tamanioDireccion;
fd_set socketsParaLectura;

enum {headerError, headerHandshake, headerScript};

void configurarServidor(){
	// Definimos la direccion, creamos el socket, bindeamos y ponemos a escuchar nuevas conexiones
	direccion = crearDireccionParaServidor(PUERTO);
	socketNuevasConexiones = socket_w();
	bind_ws(socketNuevasConexiones,&direccion);
	listen_w(socketNuevasConexiones);
	tamanioDireccion=sizeof(direccion);
}

void inicializarClientes(){
	// Inicializamos en desconectado = 0
	int i;
	for (i = 0; i < MAXCLIENTS; i++){
		socketCliente[i]=0;
	}
}

void agregarCliente(int cliente){
	// Agregamos un cliente en la primera posicion libre del array de clientes
	int i=0;
	for (i = 0; i < MAXCLIENTS; i++) {
		if( socketCliente[i] == 0 )	{
			socketCliente[i] = cliente;
			printf("Añadido a la lista de sockets como %d\n" , i);
			break;
		}
	}
}

void quitarCliente(int i){
	// Liberamos del array al cliente y cerramos su socket
	getpeername(socketCliente[i] , (struct sockaddr*)&direccion , (socklen_t*)&tamanioDireccion);
	printf("Invitado desconectado , ip %s , puerto %d \n" , inet_ntoa(direccion.sin_addr) , ntohs(direccion.sin_port));
	close(socketCliente[i]);
	socketCliente[i] = 0;
}

void procesarNuevasConexiones(){
	// Aceptamos nueva conexion
	int socketNuevoCliente;
	socketNuevoCliente = accept(socketNuevasConexiones, (struct sockaddr *)&direccion, (socklen_t*)&tamanioDireccion);
	printf("Nueva conexión , socket %d , ip is : %s , puerto : %d \n" , socketNuevoCliente , inet_ntoa(direccion.sin_addr) , ntohs(direccion.sin_port));
	agregarCliente(socketNuevoCliente);
}

int tieneLectura(int socket){
	return FD_ISSET(socket,&socketsParaLectura);
}

int incorporarSockets(){
	// Reseteamos y añadimos al set los sockets disponibles
	int i,filedes,max_filedes;
	FD_ZERO(&socketsParaLectura);
	FD_SET(socketNuevasConexiones, &socketsParaLectura);
	max_filedes=socketNuevasConexiones;
	for (i = 0; i < MAXCLIENTS; i++){
		filedes=socketCliente[i];
		if (filedes>0)
			FD_SET(socketCliente[i],&socketsParaLectura);

		if (filedes>max_filedes)
			max_filedes=filedes;
	}
	return max_filedes;
}

void procesarHeader(int cliente, char* header){
	// Segun el protocolo procesamos el header del mensaje recibido
	int payload_size;
	char* payload;

	switch(atoi(header)) {

	case headerError:
		printf("Header de Error\n");
		quitarCliente(cliente);
		break;

	case headerHandshake:
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

	case headerScript: /*A implementar*/ break;
	/* Agregar futuros casos */

	default: printf("Llego cualquier cosa\n"); break;
	}
}

int main(void) {

	int mayorDescriptor, i;
	struct timeval espera; 		// Periodo maximo de espera del select
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	char header[HEADER_SIZE];

	configurarServidor();
	inicializarClientes();
	puts("Esperando conexiones ...");

	while(1){
		mayorDescriptor = incorporarSockets();
		select( mayorDescriptor + 1 , &socketsParaLectura , NULL , NULL , &espera);

		if (tieneLectura(socketNuevasConexiones))
			procesarNuevasConexiones();

		for (i = 0; i < MAXCLIENTS; i++){
			if (tieneLectura(socketCliente[i]))	{
				if (read( socketCliente[i] , header, HEADER_SIZE) == 0)
					quitarCliente(i);
				else
					procesarHeader(i,header);
			}
		}
	}
	return EXIT_SUCCESS;
}
