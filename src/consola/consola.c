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

struct sockaddr_in direccionServidor;
int cliente;

void conectarANucleo()
{
	direccionServidor = crearDireccionParaCliente(8080);
	cliente = socket_w();
	connect_w(cliente, &direccionServidor);
}

int getHeader()
{
	return recv_nowait_ws(cliente,sizeof(handshake_t));
}

void handshakear()
{

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
	programa = fopen(path,"r");
	free(path);
	conectarANucleo();
	handshakear();


	fclose(programa);
	return 0;
}
