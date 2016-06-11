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
	char* cadena = string_new();
	if (esVariableDeclarada(head, append(cadena,variable))) {
		free(cadena);
		return DECLARADA;
	} else {
		if (esParametro(variable)) {
			free(cadena);
			return PARAMETRO;
		}
	}
	free(cadena);
	return NOEXISTE;
}

/**
 * Solo invocar desde las primitivas porque pide siempre size = 4.
 */
void enviar_direccion_umc(t_puntero direccion) {

	int pagina = (int)(direccion/tamanioPaginas);
	int offset = direccion % tamanioPaginas;
	int size=sizeof(int);

	enviar_solicitud(pagina,offset,size);
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
