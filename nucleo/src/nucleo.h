/*
 * nucleo.h
 *
 *  Created on: 1/5/2016
 *      Author: utnso
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_

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
#include <string.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <math.h>
#include <parser/metadata_program.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"
#include "serializacion.h"

/* ---------- INICIO DEBUG ---------- */
// Es util para debugear sin tener una consola extra con UMC abierto.
#define DEBUG_IGNORE_UMC true
#define DEBUG_IGNORE_UMC_PAGES true
/* ---------- INICIO DEBUG ---------- */
// Para que rompan las listas y vectores
#define SIN_ASIGNAR -1

// Globales de servidor
int socketConsola, socketCPU;
int activadoCPU, activadoConsola; //No hace falta iniciarlizarlas. Lo hacer la funcion permitir reutilizacion ahora.
struct sockaddr_in direccionConsola, direccionCPU;
unsigned int tamanioDireccionConsola, tamanioDireccionCPU;

// Globales de cliente
struct sockaddr_in direccionParaUMC;
int umc; //se usa para ser cliente de UMC

// Hilos
pthread_t crearProcesos; // 1..n instancias. Este hilo crear procesos nuevos para evitar un bloqueo del planificador. Sin este hilo, el principal llama al hilo UMC para pedir paginas y debe bloquearse hasta tener la respuesta!
pthread_attr_t detachedAttr; // Config para todos los hilos!
// Semaforos
pthread_mutex_t lockProccessList, lock_UMC_conection;

// Posibles estados de un proceso
typedef enum {
	NEW, READY, EXEC, BLOCK, EXIT
} t_proceso_estado;

typedef enum {
	FIFO, RR
} t_planificacion;
t_planificacion algoritmo;

typedef struct {
	int consola; // Indice de socketCliente
	int cpu; // Indice de socketCliente, legible solo cuando estado sea EXEC
	t_proceso_estado estado;
	t_PCB PCB;
} t_proceso;

t_queue* colaListos;
t_queue* colaSalida;
t_queue* colaCPU; //Mejor tener una cola que tener que crear un struct t_cpu que diga la disponibilidad
t_list* listaProcesos;
// Falta la cola de bloqueados para cada IO

typedef struct customConfig {
	int puertoConsola;
	int puertoCPU;
	int quantum; //TODO que sea modificable en tiempo de ejecucion si el archivo cambia
	int queantum_sleep; //TODO que sea modificable en tiempo de ejecucion si el archivo cambia
	// TODOS TIENEN QUE SER CHAR**, NO ES LO MISMO LEER 4 BYTES QUE 1
	char** sem_ids;
	char** semInit;
	char** io_ids;
	char** ioSleep;
	char** sharedVars;
	// Agrego cosas que no esta en la consigna pero necesitamos
	int puertoUMC;
	char* ipUMC;
} customConfig_t;

customConfig_t config;
t_config* configNucleo;

//Firmas
int getHandshake();
void warnDebug();
void imprimirVariable(int cliente);
void imprimirTexto(int cliente);
void cargarCFG();
void configHilos();
void procesarHeader(int cliente, char *header) ;
struct timeval newEspera();
void finalizar();
//UMC
bool pedirPaginas(int PID, char* codigo);
void establecerConexionConUMC();
void conectarAUMC();
void handshakearUMC();
//Procesos
int crearProceso(int consola);
void cargarProceso(int consola);
void ejecutarProceso(int PID, int cpu);
void rechazarProceso(int PID);
void bloquearProceso(int PID, int IO);
void desbloquearProceso(int PID);
void finalizarProceso(int PID);
void destruirProceso(int PID);
void actualizarPCB(t_PCB PCB);
bool terminoQuantum(t_proceso* proceso);
void expulsarProceso(t_proceso* proceso);
int cantidadProcesos();
void planificacionFIFO();
void planificacionRR();
void planificarProcesos();
int getConsolaAsociada(int cliente);
char* getScript(int consola);
void asignarMetadataProceso(t_proceso* p, char* codigo);
//Tests
int test_nucleo();
void test_cicloDeVidaProcesos();
void test_obtenerMetadata();

#endif /* NUCLEO_H_ */
