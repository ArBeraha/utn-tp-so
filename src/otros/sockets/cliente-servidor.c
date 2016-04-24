#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "cliente-servidor.h"

int getMaxClients()
{
	return MAXCLIENTS;
}

int socket_w()
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

void send_w(int cliente, char* msg, int msgSize)
{
	send(cliente, msg, msgSize, 0);
}

struct sockaddr_in crearDireccionParaCliente(unsigned short PORT)
{
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(PORT);
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

struct sockaddr_in crearDireccionParaServidor(unsigned short PORT)
{
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(PORT);
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

///-> Definidas por Ariel en nucleo

void configurarServidor(unsigned short PORT){
	// Definimos la direccion, creamos el socket, bindeamos y ponemos a escuchar nuevas conexiones
	direccion = crearDireccionParaServidor(PORT);
	socketNuevasConexiones = socket_w();
	activado=1;
	permitirReutilizacion(socketNuevasConexiones,&activado);
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

