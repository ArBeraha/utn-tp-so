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
#include <stdbool.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/log.h"
#include "../otros/commonTypes.h"

// Globales de servidor
int socketConsola, socketCPU, mayorDescriptor;
int activadoCPU, activadoConsola; //No hace falta iniciarlizarlas. Lo hacer la funcion permitir reutilizacion ahora.
struct sockaddr_in direccionConsola, direccionCPU;
unsigned int tamanioDireccionConsola, tamanioDireccionCPU;

// Globales de cliente
struct sockaddr_in direccionParaUMC;
int cliente; //se usa para ser cliente de UMC

#define PUERTOCONSOLA 8080
#define PUERTOCPU 8088

#define UMC_PORT 8081


#define SIN_ASIGNAR -1 // Para que rompan las listas y vectores

// Posibles estados de un proceso
typedef enum {
	NEW, READY, EXEC, BLOCK, EXIT
} t_proceso_estado;

typedef struct {
	char* codigo;
	int consola; // Indice de socketCliente
	int cpu; // Indice de socketCliente, legible solo cuando estado sea EXEC
	t_proceso_estado estado;
	struct t_PCB PCB;
} t_proceso;

t_queue* colaListos;
t_queue* colaSalida;
t_queue* colaCPU; //Mejor tener una cola que tener que crear un struct t_cpu que diga la disponibilidad
t_list* listaProcesos;
// Falta la cola de bloqueados para cada IO

/* INICIO PARA PLANIFICACION */
bool pedirPaginas(int PID, char* codigo){
	int respuesta=false;
	// todo: Calcular cantidad de paginas
	// todo: preguntar por cantidad a umc
	return respuesta;
}

int crearProceso(int consola) {
	t_proceso* proceso = malloc(sizeof(t_proceso));
	proceso->PCB.PID = list_add(listaProcesos, proceso);
	proceso->PCB.PC = SIN_ASIGNAR;
	proceso->PCB.SP = SIN_ASIGNAR;
	proceso->estado = NEW;
	proceso->consola = consola;
	proceso->cpu = SIN_ASIGNAR;
	if(!pedirPaginas(proceso->PCB.PID, proceso->codigo)) { // Si la UMC me rechaza la solicitud de paginas, rechazo el proceso
		rechazarProceso(proceso->PCB.PID);
		log_info(bgLogger, "Se rechazo el proceso %d!",proceso->PCB.PID);
	}
	return proceso->PCB.PID;
}

void ejecutarProceso(int PID, int cpu){
	t_proceso* proceso = list_get(listaProcesos,PID);
	if (proceso->estado != READY)
		log_warning(activeLogger, "Ejecucion del proceso %d sin estar listo!",PID);
	proceso->estado = EXEC;
	proceso->cpu = cpu;
	// todo: mandarProcesoCpu(cpu, proceso->PCB);
};

void rechazarProceso(int PID){
	t_proceso* proceso = list_remove(listaProcesos,PID);
	if (proceso->estado != NEW)
		log_warning(activeLogger, "Se esta rechazando el proceso %d ya aceptado!",PID);
	send(socketCliente[proceso->consola], intToChar(HeaderConsolaFinalizarRechazado), 1, 0); // Le decimos adios a la consola
	quitarCliente(proceso->consola); // Esto no es necesario, ya que si la consola funciona bien se desconectaria, pero quien sabe...
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	free(proceso); // Destruir Proceso y PCB
}

void finalizarProceso(int PID){
	t_proceso* proceso = list_get(listaProcesos,PID);
	queue_push(colaCPU,(int*)proceso->cpu); // Disponemos de nuevo de la CPU
	proceso->cpu = SIN_ASIGNAR;
	proceso->estado= EXIT;
	queue_push(colaSalida,PID);
}

void destruirProceso(int PID){
	t_proceso* proceso = list_remove(listaProcesos,PID);
	if (proceso->estado != EXIT)
		log_warning(activeLogger, "Se esta destruyendo el proceso %d que no libero sus recursos!",PID);
	send(socketCliente[proceso->consola], intToChar(HeaderConsolaFinalizarNormalmente), 1, 0); // Le decimos adios a la consola
	quitarCliente(proceso->consola); // Esto no es necesario, ya que si la consola funciona bien se desconectaria, pero quien sabe...
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	free(proceso); // Destruir Proceso y PCB
}

void planificarProcesos(){
	//FIFO por ahora
	if (!queue_is_empty(colaListos) && !queue_is_empty(colaCPU))
		ejecutarProceso(queue_pop(colaListos),queue_pop(colaCPU));

	if (!queue_is_empty(colaSalida))
		destruirProceso(queue_pop(colaSalida));
}

void bloquearProceso(int PID, int IO){
	t_proceso* proceso = list_get(listaProcesos,PID);
	if (proceso->estado != EXEC)
		log_warning(activeLogger, "Bloqueo inadecuado del proceso %d!",PID);
	proceso->estado = BLOCK;
	queue_push(colaCPU,proceso->cpu); // Disponemos de la CPU
	proceso->cpu = SIN_ASIGNAR;
	// todo: Añadir a la cola de ese IO
}

void desbloquearProceso(int PID){
	t_proceso* proceso = list_get(listaProcesos,PID);
	if (proceso->estado != BLOCK)
		log_warning(activeLogger, "Desbloqueando el proceso %d sin estar bloqueado!",PID);
	proceso->estado = READY;
	queue_push(colaListos,PID);
}
/* FIN PARA PLANIFICACION */

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

void imprimirVariable() {
	char* msgValue = recv_waitall_ws(cliente, sizeof(ansisop_var_t));
	char* name = recv_waitall_ws(cliente, sizeof(char));
	send_w(NULL, headerToMSG(HeaderImprimirVariable), 1); //fixme: null es la consola asociada al proceso que corre en la cpu que manda el mensaje!
	send_w(NULL, msgValue, sizeof(ansisop_var_t)); //fixme: null es la consola asociada al proceso que corre en la cpu que manda el mensaje!
	send_w(NULL, name, sizeof(char)); //fixme: null es la consola asociada al proceso que corre en la cpu que manda el mensaje!;
	free(msgValue);
	free(name);
}

void imprimirTexto() {
	char* msgSize = recv_waitall_ws(cliente, sizeof(int));
	int size = charToInt(msgSize);
	char* texto = recv_waitall_ws(cliente, size);
	log_debug(bgLogger, "%s", texto);
	send_w(NULL, headerToMSG(HeaderImprimirTexto), 1); //fixme: null es la consola asociada al proceso que corre en la cpu que manda el mensaje!
	send_w(NULL, intToChar(size), 1); //fixme: null es la consola asociada al proceso que corre en la cpu que manda el mensaje!
	send_w(NULL, texto, size); //fixme: null es la consola asociada al proceso que corre en la cpu que manda el mensaje!
	free(msgSize);
	free(texto);
}

void procesarHeader(int cliente, char *header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger, "Llego un handshake");
		payload_size = 1;
		payload = malloc(payload_size);
		read(socketCliente[cliente], payload, payload_size);
		log_debug(bgLogger, "Llego un mensaje con payload %d",
				charToInt(payload));
		if ((charToInt(payload) == SOYCONSOLA)
				|| (charToInt(payload) == SOYCPU)) {
			log_debug(bgLogger,
					"Es un cliente apropiado! Respondiendo handshake");
			send(socketCliente[cliente], intToChar(SOYNUCLEO), 1, 0);
		} else {
			log_error(activeLogger,
					"No es un cliente apropiado! rechazada la conexion");
			log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
			quitarCliente(cliente);
		}
		free(payload);
		break;

	case HeaderImprimirVariable:
		imprimirVariable();
		break;

	case HeaderImprimirTexto:
		imprimirTexto();
		break;

	case HeaderScript: /*A implementar*/
		break; //TODO
		/* Agregar futuros casos */

	default:
		log_error(activeLogger, "Llego cualquier cosa.");
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
		quitarCliente(cliente);
		break;
	}
}

struct timeval newEspera() {
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}

/* INICIO PARA UMC */
int getHandshake() {
	char* handshake = recv_nowait_ws(cliente, 1);
	return charToInt(handshake);
}

void handshakear() {
	char *hand = string_from_format("%c%c", HeaderHandshake, SOYNUCLEO);
	send_w(cliente, hand, 2);

	log_debug(bgLogger, "UMC handshakeo.");
	if (getHandshake() != SOYUMC) {
		perror("Se esperaba conectarse a la UMC.");
	} else
		log_debug(bgLogger, "Núcleo recibió handshake de UMC.");
}

void conectarALaUMC() {
	direccionParaUMC = crearDireccionParaCliente(UMC_PORT); //todo: reemplazar por el que se carga desde la config
	cliente = socket_w();
	connect_w(cliente, &direccionParaUMC);
}

void realizarConexionConUMC() {
	conectarALaUMC();
	log_info(activeLogger, "Conexion al nucleo correcta :).");
	handshakear();
	log_info(activeLogger, "Handshake con UMC finalizado exitosamente.");
}

void manejarUMC() {
	log_debug(bgLogger, "Hilo para solicitudes de UMC inicializado.");
	realizarConexionConUMC();
	// TODO:
	// 1. reservar memoria (las consultas a memoria reservada las hacen las cpus, pero la reserva la hace el nucleo)
	// 2. Esperar respuesta sobre si se pudo reservar o no.
	// Volver a 1., porque pueden volver a pedirme que reserve memoria ...
}
/* FIN PARA UMC */

int main(void) {

	int i;
	struct timeval espera = newEspera(); // Periodo maximo de espera del select
	char header[1];
	listaProcesos = list_create();
	colaCPU = queue_create();
	colaListos = queue_create();
	colaSalida = queue_create();


	crearLogs("Nucleo", "Nucleo");

	configurarServidorExtendido(&socketConsola, &direccionConsola,
			PUERTOCONSOLA, &tamanioDireccionConsola, &activadoConsola);
	configurarServidorExtendido(&socketCPU, &direccionCPU, PUERTOCPU,
			&tamanioDireccionCPU, &activadoCPU);

	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones ...");

	// Me conecto a la umc y hago el handshake
	//pthread_t UMC; //hilo para UMC. Asi si UMC tarda, Nucleo puede seguir manejando CPUs y consolas sin bloquearse.
	//pthread_create(&UMC, NULL, (void*)manejarUMC, NULL);

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketConsola, &socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);

		mayorDescriptor = incorporarClientes();
		if ((socketConsola>mayorDescriptor) || (socketCPU>mayorDescriptor))
			if (socketConsola > socketCPU)
				mayorDescriptor = socketConsola;
			else
				mayorDescriptor = socketCPU;

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, &espera);

		if (tieneLectura(socketConsola))
			procesarNuevasConexionesExtendido(&socketConsola);

		if (tieneLectura(socketCPU))
			procesarNuevasConexionesExtendido(&socketCPU);

		for (i = 0; i < getMaxClients(); i++) {
			if (tieneLectura(socketCliente[i])) {
				if (read(socketCliente[i], header, 1) == 0) {
					log_error(activeLogger,
							"Se rompio la conexion. Read leyó 0 bytes");
					quitarCliente(i);
				} else
					procesarHeader(i, header);
			}
		}

		planificarProcesos();
	}

	destruirLogs();

	list_destroy(listaProcesos);
	queue_destroy(colaCPU);
	queue_destroy(colaListos);
	queue_destroy(colaSalida);

	return EXIT_SUCCESS;
}
