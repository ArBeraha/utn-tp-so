#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "cliente-servidor.h"

int socket_w()
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

void send_w(int cliente, char* msg, int msgSize)
{
	send(cliente, msg, msgSize, 0);
}

struct sockaddr_in crearDireccionParaCliente()
{
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);
	return direccionServidor;
}

void connect_w(int cliente, struct sockaddr_in* direccionServidor)
{
	if (connect(cliente, (void*) direccionServidor, sizeof(*direccionServidor)) != 0)
	{
		perror("No se pudo conectar");
		exit(1);
	}
}

void permitirReutilizacion(int servidor, int* activado)
{
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, activado, sizeof(*activado));
}

char* recv_nowait_ws(int cliente, int msgSize)
{
	return recv_con_criterio_ws(cliente,msgSize,0);
}

char* recv_waitall_ws(int cliente, int msgSize)
{
	return recv_con_criterio_ws(cliente,msgSize,MSG_WAITALL);
}

char* recv_con_criterio_ws(int cliente, int msgSize, int msgCriterio)
{
	char* buffer = malloc(msgSize);
	int bytesRecibidos = recv(cliente, buffer, msgSize, msgCriterio);
	if (bytesRecibidos <= 0)
	{
		perror("recv devolvio un numero menor que cero");
		exit(1);
	}
	//buffer[msgSize-1]='\0';
	return buffer;
}

void listen_w(int servidor)
{
	listen(servidor, 100);
	//printf("Estoy escuchando...\n"); TODO deberia ser un log.
}

struct sockaddr_in crearDireccionParaServidor()
{
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(8080);
	return direccionServidor;
}

void bind_ws(int servidor, struct sockaddr_in* direccionServidor)
{
	if( bind(servidor, (void*)direccionServidor, sizeof(*direccionServidor)) != 0 )
	{
		perror("Fallo el bind");
		exit(1);
	}
}
