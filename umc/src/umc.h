/*
 * umc.h
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#ifndef UMC_H_
#define UMC_H_

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
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"
#include <math.h>
#include "CUnit/Basic.h"
#include <mcheck.h>
#include "serializacion.h"
#include "hilos.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

pthread_mutex_t mutexClientes;

#define MUTEXCLIENTES(CONTENIDO) \
	MUTEX(CONTENIDO,mutexClientes);
pthread_mutex_t mutexSwap;
#define MUTEXSWAP(CONTENIDO) \
	MUTEX(CONTENIDO,mutexSwap);
#define HILO void*

typedef struct customConfig {
	int puerto_umc_nucleo;
	int puerto_swap;
	int puerto_cpu;

	int cantidad_marcos;
	int tamanio_marco;
	int mostrar_paginas;
	int mostrar_tlb;
	int mostrar_MemoriaAlFinalizar;
	int mostrar_paginas_todas;

	int entradas_tlb;
	int retardo;
	char* ip_swap;
	char* algoritmo_paginas;
	int marcos_x_proceso;
} customConfig_t;

customConfig_t config;
t_config* configUmc;


typedef struct tlbStruct{
	int pid,
		pagina,
		marcoUtilizado,
		contadorTiempo;
}tlb_t;

typedef struct{
	 int pid,
		 paginaRequerida,
		 offset,
		 cantBytes;
}pedidoLectura_t;

typedef struct{ //No hace falta indicar el numero de la pagina, es la posicion
	int nroPagina;
	int marcoUtilizado;
	char bitPresencia;
	char bitModificacion;
	char bitUso;
}tablaPagina_t;

typedef struct{
	int pid;
	t_list* listaPaginas;
}tabla_t;

typedef struct{
	int pid;
	int posicion;
}ultimaSacada_t;

//Globales

int socketCPU, socketNucleo;
int activadoCPU, activadoNucleo; //No hace falta iniciarlizarlas. Lo hacer la funcion permitir reutilizacion ahora.
struct sockaddr_in direccionCPU, direccionNucleo;
unsigned int tamanioDireccionCPU, tamanioDireccionNucleo;

typedef int ansisop_var_t;

int swapServer;

t_log *activeLogger, *bgLogger;
char* memoria;

char* pedidoPaginaPid ;
char* pedidoPaginaTamanioContenido;

t_list* listaTablasPaginas;
tlb_t* tlb;

unsigned int* vectorMarcosOcupados;

int tamanioMemoria;

t_log *dump;


char* vectorClientes;

int retardoMemoria;

pthread_mutex_t lock_accesoMemoria;
pthread_mutex_t lock_accesoTabla;
pthread_mutex_t lock_accesoTlb;
pthread_mutex_t lock_accesoMarcosOcupados;
pthread_mutex_t lock_accesoUltimaPos;
pthread_mutex_t lock_accesoLog;
pthread_mutex_t lock_accesoSwap;


//pthread_t vectorHilosCpu[MAXCLIENTS];

pthread_t hiloParaCpu;

pthread_t hiloRecibirComandos;
pthread_attr_t detachedAttr;

int tiempo;

//int vectorUltimaPosicionSacada[MAXCLIENTS];
t_list* listaUltimaPosicionSacada;

int paginas_stack;


//Prototipos

//void reemplazarEntradaConLru(tablaPagina_t* pagina,int pidParam,int &pid,int &pag,int &marco);
void reemplazarEntradaConClock(tablaPagina_t* pagina,int pidParam);
void reemplazarEntradaConModificado(tablaPagina_t* pagina,int pidParam);
int estaEnTlb(pedidoLectura_t pedido);
int buscarEnTlb(pedidoLectura_t pedido);
int existePidEnListadeTablas(int pid);
int existePaginaBuscadaEnTabla(int pag, tabla_t* tablaPaginaBuscada);
char* buscarMarco(int marcoBuscado, pedidoLectura_t pedido);
int buscarPrimerMarcoLibre();
int cantidadMarcosLibres();

int buscarEnSwap(pedidoLectura_t pedido, t_cliente cliente);
void agregarAMemoria(pedidoLectura_t pedido, char* contenido, t_cliente cliente);

char* devolverPedidoPagina(pedidoLectura_t pedido,t_cliente cliente);   // todos estos volver a devolver void, devuelven cosas para testear

char* almacenarBytesEnUnaPagina(pedidoLectura_t pedido, char* buffer, t_cliente cliente);
void finalizarPrograma(int idPrograma);
int inicializarPrograma(int idPrograma, char* contenido, int tamanio);
int reservarPagina(int,int);

void imprimirRegionMemoriaStack(char* region, int size);
void imprimirRegionMemoriaCodigo(char* region, int size);

void devolverTodasLasPaginas();
void devolverPaginasDePid(int pid);
void devolverTodaLaMemoria();
void devolverMemoriaDePid(int pid);
void inicializarTlb();
void fRetardo();
void dumpEstructuraMemoria();
void dumpContenidoMemoria();
void flushTlb();
void flushTlbDePid(int pid);
void sacarPosDeTlb(int pos);
void flushMemory();
void imprimirRegionMemoriaCodigoConsola(char* contenido,int size);
void imprimirRegionMemoriaStackConsola(char* contenido,int size);
void imprimirRegionMemoriaCodigoLogDump(char* contenido,int size);
void imprimirRegionMemoriaStackLogDump(char* contenido,int size);

HILO recibirComandos();

void servidorCPUyNucleoExtendido();
void servidorCPUyNucleo();
int  getHandshake();
void handshakearASwap();
void conectarASwap(); //MOTHER OF EXPRESIVIDAD... ver si puedo mejorar estos nombres

void procesarHeader(t_cliente cliente, char *header);

void crearMemoriaYTlbYTablaPaginas();

void test2();

tabla_t* buscarTabla(int pidBusca);
int cantPaginasEnMemoriaDePid(int pid);
int sacarConClock(int pid);
int sacarConModificado(int pid);
void enviarASwap(int pid,tablaPagina_t* pagina);
void sacarDeMemoria(tablaPagina_t* pagina, int pid);
void ponerBitModif1(int pid,int nroPagina);
int cantPaginasDePid(int pid);
void agregarATlb(tablaPagina_t* pagina ,int pid);
void sacarMarcosOcupados(int idPrograma);
int primerNumeroPaginaLibre(int pid);
void finalizar();
void mostrarTlb();
int buscarPosicionTabla(int idPrograma);
int buscarUltimaPosSacada(int pid);
void cambiarUltimaPosicion(int pidParam, int ultima);

int buscarPosicionTabla(int idPrograma);
void sacarMarcosOcupados(int idPrograma);



char* devolverBytes(pedidoLectura_t pedido, t_cliente cliente);
char* almacenarBytes(pedidoLectura_t pedido, char* buffer,t_cliente cliente);



//Fin prototipos


HILO main2();
HILO hiloDedicado(int indice);
void cargarCFG();
void atenderHandshake(t_cliente cliente);
void ejemploSWAP(t_cliente cliente);
void operacionScript(t_cliente cliente);

#endif /* UMC_H_ */
