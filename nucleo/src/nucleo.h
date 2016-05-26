/*
 * base.h
 *
 *  Created on: 25/5/2016
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
pthread_t hiloBloqueos;
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
	t_PCB* PCB;
} t_proceso;
t_queue* colaListos;
t_queue* colaSalida;
t_queue* colaCPU;
t_list* listaProcesos;
t_dictionary* tablaIO;
typedef enum {
	ACTIVE, INACTIVE
} t_IO_estado;
typedef struct {
	int retardo;
	t_IO_estado estado;
	t_queue* cola;
} t_IO;
t_dictionary* tablaSEM;
t_dictionary* tablaGlobales;
typedef struct {
	t_IO* IO;
	int PID;
} t_bloqueo;
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

//Nucleo
void cargarCFG();
void configHilos();
void procesarHeader(int cliente, char *header);
struct timeval newEspera();
void finalizar();
int getHandshake();
void warnDebug();
//UMC
bool pedirPaginas(int PID, char* codigo);
void enviarCodigo(int PID, char* codigo);
void establecerConexionConUMC();
void conectarAUMC();
void handshakearUMC();
//Consola
int getConsolaAsociada(int cliente);
//Procesos
int crearProceso(int consola);
void cargarProceso(int consola);
char* getScript(int consola);
void ejecutarProceso(int PID, int cpu);
void rechazarProceso(int PID);
void bloquearProceso(int PID, char* IO);
void desbloquearProceso(int PID);
void finalizarProceso(int PID);
void destruirProceso(int PID);
void actualizarPCB(t_PCB PCB);
void expulsarProceso(t_proceso* proceso);
void asignarMetadataProceso(t_proceso* p, char* codigo);
//Planificacion
int cantidadProcesos();
void planificacionFIFO();
void planificacionRR();
void planificarProcesos();
void planificarIO(char* io_id, t_IO* io);
bool terminoQuantum(t_proceso* proceso);
void asignarCPU(t_proceso* proceso, int cpu);
void desasignarCPU(t_proceso* proceso);
void bloqueo(t_bloqueo* info);
//Primitivas
void signalSemaforo(int cliente);
void waitSemaforo(int cliente);
void asignarCompartida(int cliente);
void devolverCompartida(int cliente);
void imprimirVariable(int cliente);
void imprimirTexto(int cliente);
void entradaSalida(int cliente);
//Tests
int testear(int(*suite)(void));
int test_nucleo();
void test_cicloDeVidaProcesos();
void test_obtenerMetadata();

#endif /* NUCLEO_H_ */
