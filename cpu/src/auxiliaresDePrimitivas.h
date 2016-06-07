/*
 * auxiliaresDePrimitivas.h
 *
 *  Created on: 6/6/2016
 *      Author: utnso
 */

#ifndef AUXILIARES_DE_PRIMITIVAS_H_
#define AUXILIARES_DE_PRIMITIVAS_H_

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

/* ------Auxiliares ----- */
bool esVariableDeclarada(t_stack_item* item, t_nombre_variable* variable);
bool esParametro(t_nombre_variable variable);
int tipoVaraible(t_nombre_variable variable, t_stack_item* head);
void enviar_direccion_umc(t_puntero direccion);
bool existeLabel(t_nombre_etiqueta etiqueta);
int digitosDe(t_valor_variable valor);


#endif /* AUXILIARES_DE_PRIMITIVAS_H_ */
