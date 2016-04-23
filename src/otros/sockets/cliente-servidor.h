/*
 * cliente-servidor.h
 *
 *  Created on: 10/4/2016
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


#ifndef CLIENTE_SERVIDOR_H_
#define CLIENTE_SERVIDOR_H_

#define MAXCLIENTS 30
#define PUERTO 8080
#define HEADER_SIZE 1

struct sockaddr_in direccion;
int socketNuevasConexiones, socketCliente[MAXCLIENTS], tamanioDireccion;
fd_set socketsParaLectura;

/* ¿Como funciona esto?
 * Las funciones que solo son wrappers se llaman igual que la original con _w.
 * Las funciones que hacen un chequeo de algun tipo para errores, se llaman igual con _s (s de safe. Alguna vez escuche de funciones de algunas bibliotecas de C que siguen esta convencion de _s. Por ahi flashee cualquiera D:
 * Las que cumplen las dos cosas anteriores, _ws.
 * Algunas que no son wrappers o los wrappers de recv, tienen otros nombres. Siempre respetando lo de los _ que dije arriba.
 */

int socket_w();
void send_w(int cliente, char* msg, int msgSize);
struct sockaddr_in crearDireccionParaCliente(unsigned short PORT);
void connect_w(int cliente, struct sockaddr_in* direccionServidor);
void permitirReutilizacion(int servidor, int* activado);
char* recv_nowait_ws(int cliente, int msgSize);
char* recv_waitall_ws(int cliente, int msgSize);
void listen_w(int servidor);
struct sockaddr_in crearDireccionParaServidor(unsigned short PORT);
void bind_ws(int servidor, struct sockaddr_in* direccionServidor);

//Esta funcion es mas que nada algo interno, es mas comodo usar recv_nowait_ws() y recibir_waitall_ws(). Si se quiere usar desde fuera de este cliente-servidor.c es totalmente valido.
char* recv_con_criterio_ws(int cliente, int msgSize, int msgCriterio);
#endif /* CLIENTE_SERVIDOR_H_ */

//xAriel
void configurarServidor();
void inicializarClientes();
void agregarCliente(int cliente);
void quitarCliente(int i);
void procesarNuevasConexiones();
int tieneLectura(int socket);
int incorporarSockets();



