/*
 * auxiliaresDePrimitivas.c
 *
 *  Created on: 6/6/2016
 *      Author: utnso
 */

#include "auxiliaresDePrimitivas.h"

bool esVariableDeclarada(t_stack_item* item, t_nombre_variable* variable) {
	return dictionary_has_key(item->identificadores, variable);
}

bool esParametro(t_nombre_variable variable) {
	return (variable >= '0' && variable <= '9');
}

/**
 * Retorna un valor segun si fue declarada localmente, recibida por parametro o no existe.
 */
int tipoVaraible(t_nombre_variable variable, t_stack_item* head) {
	if (esVariableDeclarada(head, &variable)) {
		return DECLARADA;
	} else {
		if (esParametro(variable)) {
			return PARAMETRO;
		}
	}
	return NOEXISTE;
}

void enviar_direccion_umc(t_puntero direccion) {

	t_stack_item* stackItem = stack_get(pcbActual->SP, direccion);
	t_pedido pedido = stackItem->valorRetorno;

	char* mensaje = string_new();
	serializar_pedido(mensaje, &pedido);

	send_w(cliente_umc, mensaje, sizeof(t_pedido)); // envio el pedido [pag,offset,size]

	free(mensaje);
}

bool existeLabel(t_nombre_etiqueta etiqueta) {
	return dictionary_has_key(pcbActual->indice_etiquetas, etiqueta);
}

char *append(const char *s, char c)
{
    int len = strlen(s);
    char buf[len+2];
    strcpy(buf, s);
    buf[len] = c;
    buf[len + 1] = 0;
    return strdup(buf);
}
