/*
 * primitivas.h
 *
 *  Created on: 31/5/2016
 *      Author: utnso
 */

#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

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
#include "cpu.h"
#include "auxiliaresDePrimitivas.h"



/* ------ Primitivas ----- */
t_puntero definir_variable(t_nombre_variable variable);
t_puntero obtener_posicion_de(t_nombre_variable variable);
t_valor_variable dereferenciar(t_puntero direccion);
t_valor_variable obtener_valor_compartida(t_nombre_compartida nombreVarCompartida);
t_valor_variable asignar_valor_compartida(t_nombre_compartida nombreVarCompartida,t_valor_variable valorVarCompartida);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamar_con_retorno(t_nombre_etiqueta nombreFuncion,t_puntero dondeRetornar);
void retornar(t_valor_variable variable);
void imprimir_variable(t_valor_variable valor);
void imprimir_texto(char* texto);
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo);
void wait_semaforo(t_nombre_semaforo identificador_semaforo);
void signal_semaforo(t_nombre_semaforo identificador_semaforo);


/* ------ Funciones para usar con el parser ----- */
void inicializar_primitivas();

#endif /* PRIMITIVAS_H_ */
