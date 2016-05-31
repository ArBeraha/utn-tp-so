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


t_puntero definir_variable(t_nombre_variable variable);
bool esVariableDeclarada(t_stack_item* item, t_nombre_variable* variable);
bool esParametro(t_nombre_variable variable);
int tipoVaraible(t_nombre_variable variable, t_stack_item* head);
t_puntero obtener_posicion_de(t_nombre_variable variable);
void enviar_direccion_umc(t_puntero direccion);
t_valor_variable dereferenciar(t_puntero direccion);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtener_valor_compartida(
		t_nombre_compartida nombreVarCompartida);
t_valor_variable asignar_valor_compartida(
		t_nombre_compartida nombreVarCompartida,
		t_valor_variable valorVarCompartida);
bool existeLabel(t_nombre_etiqueta etiqueta);
void irAlLaber(t_nombre_etiqueta etiqueta);
void llamar_con_retorno(t_nombre_etiqueta nombreFuncion,
		t_puntero dondeRetornar);
t_puntero_instruccion retornar(t_valor_variable variable);
int digitosDe(t_valor_variable valor);
void imprimir(t_valor_variable valor);
void imprimir_texto(char* texto);
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal_con_semaforo(t_nombre_semaforo identificador_semaforo);

/* ------ Funciones para usar con el parser ----- */
void inicializar_primitivas();
void liberar_primitivas();


#endif /* PRIMITIVAS_H_ */
