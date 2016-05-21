/*
 * consola.h
 *
 *  Created on: 1/5/2016
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <parser/parser.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"
#include "serializacion.h"

typedef struct customConfig {
	int puertoNucleo;
	char* ipNucleo;
	int puertoUMC;
	char* ipUMC;
} customConfig_t;

t_config* configCPU;
customConfig_t config;

/*------------Macros--------------*/
#define DEBUG_IGNORE_UMC false
#define DEBUG_NO_PROGRAMS false


/*------------Variables Globales--------------*/
int cliente_nucleo; //cpu es cliente del nucleo
int cliente_umc; //cpu es cliente de umc

struct sockaddr_in direccionNucleo;   //direccion del nucleo
struct sockaddr_in direccionUmc;	  //direccion umc

AnSISOP_funciones funciones;		//funciones de AnSISOP
AnSISOP_kernel funcionesKernel;		// funciones kernel de AnSISOP

int tamanioPaginas;					//Tamaño de las paginas

t_PCB* pcbActual;
t_stack* stack;

#endif /* CPU_H_ */
