/*
 * kernel.c
 *
 *  Created on: 16/4/2016
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
#include <pthread.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/log.h"

// Globales de servidor
int socketConsola, socketCPU, mayorDescriptor;
int activadoCPU, activadoConsola; //No hace falta iniciarlizarlas. Lo hacer la funcion permitir reutilizacion ahora.
struct sockaddr_in direccionConsola, direccionCPU;
unsigned int tamanioDireccionConsola, tamanioDireccionCPU;
#define PUERTOCONSOLA 8080
#define PUERTOCPU 8088

#define UMC_PORT 8081

struct sockaddr_in direccionParaUMC;
int cliente; //se usa para ser cliente de UMC

// FIXME: error al compilar: expected ‘struct t_config *’ but argument is of type ‘struct t_config *’
// Si nadie lo sabe arreglar, podemos preguntarle a los ayudantes xD es muuuuy raro esto.
/*
typedef struct customConfig {
	int puertoConsola;
	int puertoCPU;
	int quantum; //TODO que sea modificable en tiempo de ejecucion si el archivo cambia
	int queantum_sleep; //TODO que sea modificable en tiempo de ejecucion si el archivo cambia
	char** sem_ids;
	int* semInit;
	char** io_ids;
	int* ioSleep;
	char** sharedVars;
} customConfig_t;

struct customConfig_t config;
struct t_config *configNucleo;

void cargarCFG()
{
	configNucleo = malloc(sizeof(struct t_config));
	configNucleo = config_create("nucleo.cfg");
	config.puertoConsola = config_get_int_value(configNucleo,"PUERTO_PROG");
	config.puertoCPU = config_get_int_value(configNucleo,"PUERTO_CPU");
	config.quantum = config_get_int_value(configNucleo,"QUANTUM");
	config.queantum_sleep = config_get_int_value(configNucleo,"QUANTUM_SLEEP");
	config.sem_ids = config_get_array_value(configNucleo,"SEM_IDS");
	//retorna chars, no int, pero como internamente son lo mismo, entender un puntero como a char* o a int* es indistinto
	config.semInit = config_get_array_value(configNucleo,"SEM_INIT");
	config.io_ids = config_get_array_value(configNucleo,"IO_IDS");
	//retorna chars, no int, pero como internamente son lo mismo, entender un puntero como a char* o a int* es indistinto
	config.ioSleep = config_get_array_value(configNucleo,"IO_SLEEP");
	config.sharedVars = config_get_array_value(configNucleo,"SHARED_VARS");
}
*/

void procesarHeader(int cliente, char *header){
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger,"Llego un mensaje con header %d",charToInt(header));

	switch(charToInt(header)) {

	case HeaderError:
		log_error(activeLogger,"Header de Error");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger,"Llego un handshake");
		payload_size=1;
		payload = malloc(payload_size);
		read(socketCliente[cliente] , payload, payload_size);
		log_debug(bgLogger,"Llego un mensaje con payload %d",charToInt(payload));
		if ((charToInt(payload)==SOYCONSOLA) || (charToInt(payload)==SOYCPU)){
			log_debug(bgLogger,"Es un cliente apropiado! Respondiendo handshake");
			send(socketCliente[cliente], intToChar(SOYNUCLEO), 1, 0);
		}
		else {
			log_error(activeLogger,"No es un cliente apropiado! rechazada la conexion");
			log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
			quitarCliente(cliente);
		}
		free(payload);
		break;

	case HeaderScript: /*A implementar*/ break; //TODO
	/* Agregar futuros casos */

	default:
		log_error(activeLogger,"Llego cualquier cosa.");
		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
		log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
		quitarCliente(cliente);
		break;
	}
}

struct timeval newEspera()
{
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}


/* INICIO PARA UMC */
int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,1);
	return charToInt(handshake);
}

void handshakear()
{
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYNUCLEO);
	send_w(cliente, hand, 2);

	log_debug(bgLogger,"UMC handshakeo.");
	if(getHandshake()!=SOYUMC)
	{
		perror("Se esperaba conectarse a la UMC.");
	}
	else
		log_debug(bgLogger,"Núcleo recibió handshake de UMC.");
}

void conectarALaUMC()
{
	direccionParaUMC = crearDireccionParaCliente(UMC_PORT); //todo: reemplazar por el que se carga desde la config
	cliente = socket_w();
	connect_w(cliente, &direccionParaUMC);
}

void realizarConexionConUMC()
{
	conectarALaUMC();
	log_info(activeLogger,"Conexion al nucleo correcta :).");
	handshakear();
	log_info(activeLogger,"Handshake con UMC finalizado exitosamente.");
}


void manejarUMC()
{
	log_debug(bgLogger,"Hilo para solicitudes de UMC inicializado.");
	realizarConexionConUMC();
	// TODO:
	// 1. reservar memoria (las consultas a memoria reservada las hacen las cpus, pero la reserva la hace el nucleo)
	// 2. Esperar respuesta sobre si se pudo reservar o no.
	// Volver a 1., porque pueden volver a pedirme que reserve memoria ...
}
/* FIN PARA UMC */


int main(void) {

	int i;
	struct timeval espera = newEspera(); 		// Periodo maximo de espera del select
	char header[1];

	crearLogs("Nucleo","Nucleo");

	configurarServidorExtendido(&socketConsola,&direccionConsola,PUERTOCONSOLA,&tamanioDireccionConsola,&activadoConsola);
	configurarServidorExtendido(&socketCPU,&direccionCPU,PUERTOCPU,&tamanioDireccionCPU,&activadoCPU);

	inicializarClientes();
	log_info(activeLogger,"Esperando conexiones ...");

	// Me conecto a la umc y hago el handshake
	//pthread_t UMC; //hilo para UMC. Asi si UMC tarda, Nucleo puede seguir manejando CPUs y consolas sin bloquearse.
	//pthread_create(&UMC, NULL, (void*)manejarUMC, NULL);

	while(1){
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketConsola, &socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);

		if (socketConsola>socketCPU)
			mayorDescriptor = socketConsola;
		else
			mayorDescriptor = socketCPU;

		mayorDescriptor = incorporarClientes();
		select( mayorDescriptor + 1 , &socketsParaLectura , NULL , NULL , &espera);

		if (tieneLectura(socketConsola))
			procesarNuevasConexionesExtendido(&socketConsola);

		if (tieneLectura(socketCPU))
			procesarNuevasConexionesExtendido(&socketCPU);

		for (i = 0; i < getMaxClients(); i++){
			if (tieneLectura(socketCliente[i]))	{
				procesarHeader(i,header);
			}
		}
	}

	destruirLogs();
	return EXIT_SUCCESS;
}
