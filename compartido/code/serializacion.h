/*
 * serializacion.h
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "commonTypes.h"
#include <commons/collections/list.h>
#include "CUnit/Basic.h"

int serializar_int(char* destino,int* fuente);
int deserializar_int(int* destino, char* fuente);
int serializar_variable(char* destino,t_pedido* fuente);
int deserializar_variable(t_pedido* destino, char* fuente);
int serializar_sentencia(char* destino,t_sentencia* fuente);
int deserializar_sentencia(t_sentencia* destino, char* fuente);
int serializar_list(char* destino,t_list* fuente, int pesoElemento);
int deserializar_list(t_list* destino,char* fuente, int pesoElemento);
int bytes_list(t_list* fuente, int pesoElemento);
int bytes_stack_item(t_stack_item* fuente);
int bytes_stack(t_stack* fuente);
int serializar_stack_item(char* destino, t_stack_item* fuente);
int serializar_stack(char* destino, t_stack* fuente);
int deserializar_stack_item(t_stack_item* destino, char* fuente);
int deserializar_stack(t_stack* destino, char* fuente);
int serializar_dictionary(char* destino, t_dictionary* fuente, int pesoData);
int deserializar_dictionary(t_dictionary* destino, char* fuente, int pesoData);
int bytes_dictionary(t_dictionary* fuente, int pesoData);
int serializar_PCB(char* destino,t_PCB* fuente);
int deserializar_PCB(t_PCB* destino, char* fuente);

// Debug
void imprimir_serializacion(char* serial, int largo);

// Test Call
int test_serializacion();
// Tests
void test_serializar_int();
void test_serializar_variable();
void test_serializar_sentencia();
void test_serializar_list();
void test_serializar_dictionary();
void test_serializar_stack_item();
void test_serializar_stack();
void test_serializar_PCB();

#endif /* SERIALIZACION_H_ */


