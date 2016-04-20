/*
 * consola.c
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

struct sockaddr_in direccionServidor;
int cliente;

void conectarANucleo()
{
	direccionServidor = crearDireccionParaCliente(8080);
	cliente = socket_w();
	connect_w(cliente, &direccionServidor);
}

int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,sizeof(handshake_t));
	return &handshake;
}

void handshakear()
{
	handshake_t hand = SOYCONSOLA;
	send_w(cliente, (char*)&hand, sizeof(hand));
	printf("Consola handshakeo\n");
	if(getHandshake()!=SOYNUCLEO)
	{
		perror("Se esperaba que la consola se conecte con el nucleo.\n");
	}
	printf("COnsola recibio handshake.\n");
}

int main(int argc, char* argv[])
{
	FILE* programa;
	char* path=NULL;
	printf("soy consola\n");
	if(argc==1)
	{
		printf("Ingresar archivo ansisop: ");
		scanf("%s",path);
	}
	else if(argc==2)
	{
		path = argv[1];
		free(argv[1]);
	}
	else
	{
		printf("Muchos parametros :C\n");
		printf("No poner parametros o poner solo el nombre del archivo a abrir");
		return 1;
	}
	//programa = fopen(path,"r");
	free(path);
	conectarANucleo();
	printf("Conexion al nucleo correcta :).\n");
	handshakear();

	//fclose(programa);
	return 0;
}
