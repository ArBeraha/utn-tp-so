/*
 * consola.h
 *
 *  Created on: 1/5/2016
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <commons/config.h>
#include <string.h>
#include <commons/log.h>
#include <unistd.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"

#define PATHSIZE 2048

typedef struct customConfig {
	int puertoNucleo;
	char* ipNucleo;
	int logLevel;
	int DEBUG;
	int DEBUG_LOG_OLD_REMOVE;
} customConfig_t;

char* path;
FILE* programa;
int cliente;
t_config* configConsola;
customConfig_t config;


#endif /* CONSOLA_H_ */
