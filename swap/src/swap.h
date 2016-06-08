/*
 * swap.h
 *
 *  Created on: 30/5/2016
 *      Author: utnso
 */

#ifndef SWAP_H_
#define SWAP_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"
#include "serializacion.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

typedef struct infoProceso {
	int pid;
	int posPagina;
	int cantidadDePaginas;
} t_infoProceso;
typedef struct datoPedido {
	int pid;
	int pagina;
	int tamanio;
} t_datosPedido;
typedef struct customConfig {
	int puerto_escucha;
	char* nombre_swap;
	int cantidad_paginas;
	int tamanio_pagina;
	int retardo_acceso;
	int retardo_compactacion;
} customConfig_t;
customConfig_t config;

t_config* archSwap;
t_bitarray* espacio; //Bitmap de espacio utilizado, voy a tener una posicion por cada marco
char *bitarray, *archivo;
t_list* espacioUtilizado;
int espacioDisponible, espacioOcupado, fd, archivoEnPaginas, cliente;
FILE* archivoSwap;

/*****PROTOTIPOS******/
void operacionHandshake();
void operacionIniciarProceso();
void operacionEscritura();
void operacionLectura();
void operacionFinalizar();
void asignarEspacioANuevoProceso(int, int);
void agregarProceso(int, int);
char* leerPagina(int);
void escribirPagina(int, char*);
void finalizarProceso(int);
void procesarHeader(int, char*);
int espaciosDisponibles(t_bitarray*);
int espaciosUtilizados(t_bitarray*);
void limpiarPosiciones(t_bitarray*, int, int);
void setearPosiciones(t_bitarray*, int, int);
int hayQueCompactar(int);
t_infoProceso* buscarProcesoSegunInicio(int);
t_infoProceso* buscarProcesoSegunPID(int);
int estaEnArray(t_infoProceso*);
void compactar();
void cargarCFG();
void crear_archivo();
void conectar_umc();
void inicializar();
void finalizar();
void esperar_peticiones();
void abrirArchivo();
void cerrarArchivo();
void escribirPagina(int numeroPagina, char* contenidoPagina);
char* leerPagina(int numeroPagina);
void configurarBitarray();
int buscarEspacio(int paginasAIniciar);
void limpiarEstructuras();
void moverPagina(int numeroPagina, int posicion);
void moverProceso(t_infoProceso* proceso, int nuevoInicio);
bool estaProceso(int pid);
//Debug
void imprimirPagina(int numeroPagina);
void imprimirBitarray();
int primerEspacioLibre();
//Tests
void test_escrituraYLecturaEstatica();
void test_escrituraYLecturaDinamica();
void test_compactacion();
void test_asignacionProcesos();
void test_espacioDisponibleYFinalizarProceso();
int testear(int (*suite)(void));
int test_swap();

#endif /* SWAP_H_ */
