/*
 * conexiones.h
 *
 *  Created on: 7/6/2016
 *      Author: utnso
 */

#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
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
#include "cpu.h"

/*------------Variables Globales--------------*/
int cliente_nucleo; //cpu es cliente del nucleo
int cliente_umc; //cpu es cliente de umc
struct sockaddr_in direccionNucleo;   //direccion del nucleo
struct sockaddr_in direccionUmc;	  //direccion umc


// ***** Funciones de conexiones ***** //
int getHandshake(int cli);
void warnDebug();
void conectar_nucleo();
void hacer_handshake_nucleo();
void conectar_umc();
void hacer_handshake_umc();
void establecerConexionConUMC();
void establecerConexionConNucleo();


#endif /* CONEXIONES_H_ */
