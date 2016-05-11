/*
 * serializacion.h
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <stdio.h>
#include "commonTypes.h"
#include <commons/collections/list.h>

void imprimir_serializacion(char* serial, int largo);
void serializar_int(char* destino,int* fuente);
void deserializar_int(int* destino, char* fuente);
void serializar_variable(char* destino,t_variable* fuente);
void deserializar_variable(t_variable* destino, char* fuente);
void serializar_sentencia(char* destino,t_sentencia* fuente);
void deserializar_sentencia(t_sentencia* destino, char* fuente);
void serializar_list(char* destino,t_list* fuente, int pesoElemento);
void deserializar_list(t_list* destino,char* fuente, int pesoElemento);

#endif /* SERIALIZACION_H_ */


