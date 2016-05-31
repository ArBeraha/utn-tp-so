/*
 * swap.h
 *
 *  Created on: 30/5/2016
 *      Author: utnso
 */

#ifndef SWAP_H_
#define SWAP_H_


#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/process.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"
#include <stdlib.h>
#include <stdio.h>
#include "serializacion.h"


int cliente;

typedef struct infoProceso{
	int pid;
	int numPagina;
	int posPagina;
	int cantidadDePaginas;
} t_infoProceso;

typedef struct datoPedido{
	int pid;
	int pagina;
	int tamanio;
}t_datosPedido;

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
char *bitarray;


t_list* espacioUtilizado;
int espacioDisponible;

int espacioLibre;
int espacioOcupado;


/*****PROTOTIPOS******/
//void asignarEspacioANuevoProceso(int, int);
//void agregarProceso(int, int);
//void leerPagina(int, int );
//void escribirPagina(int, int , int );
//void finalizarProceso(int);
void procesarHeader(int, char*);
int espaciosDisponibles (t_bitarray*);
int espaciosUtilizados (t_bitarray*);
void limpiarPosiciones (t_bitarray* , int, int );
void setearPosiciones (t_bitarray*, int, int );
int hayQueCompactar(int);
int estaElProceso(int);
t_infoProceso* buscarProceso(int);
int estaEnArray(t_infoProceso*);
//int buscarMarcoInicial(int);
void sacarElemento(int);
//void compactar();
//void modificarArchivo (int, int, int );

void cargarCFG();
void crear_archivo();
void conectar_umc();
void inicializar();


void asignarEspacioANuevoProceso(int, int);
void agregarProceso(int, int);
//void leerPagina(int, int);
//void escribirPagina(int, int , int );
void finalizarProceso(int);






void testSwapDeBitArray1();
void testSwapDeBitArray2();
//void testSwapDeCompactacion3();
void testFinalizarProceso1();
//void testFinalizarProceso2();
void testAgregarProceso1();
void testAgregarProceso2();


#endif /* SWAP_H_ */
