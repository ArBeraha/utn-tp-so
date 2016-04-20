/*
 * kernel.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */


#include "../otros/handshake.h"
#include "../otros/sockets/cliente-servidor.h"

int main()
{
	struct sockaddr_in direccionServidor;
	direccionServidor = crearDireccionParaServidor(8080);
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	int servidor = socket_w();
	int* i = 1;

	bind_ws(servidor,&direccionServidor);

	permitirReutilizacion(i);

	listen_w(servidor);
	printf("Estoy escuchando");

	int cliente = accept(servidor, (void*)&direccionCliente,&tamanioDireccion);

	return 0;
}
