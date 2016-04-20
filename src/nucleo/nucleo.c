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

struct sockaddr_in direccionServidor;
struct sockaddr_in direccionCliente;
int servidor, cliente;

int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,sizeof(handshake_t));
	return &handshake;
}

void handshakear()
{
	switch(getHandshake())
	{
	case SOYCONSOLA: break; // esto en algun futuro podria hacer algo.
	case SOYCPU: break; //TODO
	case SOYUMC: break; //TODO
	default: perror("Se conecto al nucleo algo que no es consola, cpu ni umc.\n");
	}
	printf("Nucleo recibio handshake\n");
	handshake_t hand = SOYNUCLEO;
	send_w(cliente, (void*)&hand, sizeof(hand));
	printf("Nucleo handshakeo\n");
}

int main()
{
	direccionServidor = crearDireccionParaServidor(8080);
	unsigned int tamanioDireccion;
	servidor = socket_w();
	int activado = 1;
	bind_ws(servidor,&direccionServidor);
	permitirReutilizacion(servidor,&activado);
	listen_w(servidor);
	printf("Estoy escuchando\n");
	cliente = accept(servidor, (void*)&direccionCliente,&tamanioDireccion);
	printf("Conexion aceptada :)");
	handshakear();
	return 0;
}
