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
int serializar_int(char* destino,int* fuente);
int deserializar_int(int* destino, char* fuente);
int serializar_variable(char* destino,t_variable* fuente);
int deserializar_variable(t_variable* destino, char* fuente);
int serializar_sentencia(char* destino,t_sentencia* fuente);
int deserializar_sentencia(t_sentencia* destino, char* fuente);
int serializar_list(char* destino,t_list* fuente, int pesoElemento);
int deserializar_list(t_list* destino,char* fuente, int pesoElemento);
int bytes_stack_item(t_stack_item* fuente);
int bytes_stack(t_stack* fuente);
int serializar_stack_item(char* destino, t_stack_item* fuente);
int serializar_stack(char* destino, t_stack* fuente);
int deserializar_stack_item(t_stack_item* destino, char* fuente);
int deserializar_stack(t_stack* destino, char* fuente);


#endif /* SERIALIZACION_H_ */


