/*
 * cliente-servidor.h
 *
 *  Created on: 10/4/2016
 *      Author: utnso
 */
#ifndef CLIENTE_SERVIDOR_H_
#define CLIENTE_SERVIDOR_H_

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
#include <string.h>
#include "serializacion.h"
#include "header.h"

#define MAXCLIENTS 300

struct sockaddr_in direccion;
int socketNuevasConexiones, tamanioDireccion, activado, mayorDescriptor;
fd_set socketsParaLectura;

typedef struct {
	int indice;
	struct sockaddr_in addr;
	socklen_t addrlen;
	int socket;
	int identidad;
	bool atentido;
	int pid;
	pthread_t hilo;
	void* proceso;
} t_cliente;

t_cliente clientes[MAXCLIENTS];

/* ¿Como funciona esto?
 * Las funciones que solo son wrappers se llaman igual que la original con _w.
 * Las funciones que hacen un chequeo de algun tipo para errores, se llaman igual con _s (s de safe. Alguna vez escuche de funciones de algunas bibliotecas de C que siguen esta convencion de _s. Por ahi flashee cualquiera D:
 * Las que cumplen las dos cosas anteriores, _ws.
 * Algunas que no son wrappers o los wrappers de recv, tienen otros nombres. Siempre respetando lo de los _ que dije arriba.
 */

int getMaxClients(); //retorna la cantidad maxima de clientes de un server
int socket_w();
void send_w(int cliente, char* msg, int msgSize);
struct sockaddr_in crearDireccionParaCliente(unsigned short PORT, char* IP);
void connect_w(int cliente, struct sockaddr_in* direccionServidor);
void permitirReutilizacion(int servidor, int* activado);
char* recv_nowait_ws(int cliente, int msgSize);
char* recv_waitall_ws(int cliente, int msgSize);
void listen_w(int servidor);
struct sockaddr_in crearDireccionParaServidor(unsigned short PORT);
void bind_ws(int servidor, struct sockaddr_in* direccionServidor);
//Esta funcion es mas que nada algo interno, es mas comodo usar recv_nowait_ws() y recibir_waitall_ws(). Si se quiere usar desde fuera de este cliente-servidor.c es totalmente valido.
char* recv_con_criterio_ws(int cliente, int msgSize, int msgCriterio);


//xAriel
void configurarServidor(unsigned short PORT);
void inicializarClientes();
int agregarCliente(t_cliente cliente);
void quitarCliente(int i);
void procesarNuevasConexiones();
int tieneLectura(int socket);
int incorporarSockets();
int charToInt(char *c);
char* intToChar(int i);
// Funciones extendidas para mayor control
int incorporarClientes();
int procesarNuevasConexionesExtendido(int* socket);
void configurarServidorExtendido(int* socket, struct sockaddr_in* dire, unsigned short PORT, unsigned int* tamanio, int* activado);
char* intToChar4(int num); // RECORDAR: liberar el puntero con free()
int char4ToInt(char* chars);
char* leerLargoYMensaje(int cliente);
void enviarLargoYString(int cliente, char* mensaje);
void enviarLargoYSerial(int cliente, int largo, char* mensaje);
void enviarHeader(int cliente, int header);
bool estaConectado(t_cliente cliente);

#endif /* CLIENTE_SERVIDOR_H_ */



