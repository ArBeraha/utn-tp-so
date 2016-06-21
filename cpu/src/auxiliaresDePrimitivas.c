/*
 * auxiliaresDePrimitivas.c
 *
 *  Created on: 6/6/2016
 *      Author: utnso
 */

#include "auxiliaresDePrimitivas.h"

/**
 * Como retorna un string, hacer un free!
 */
char* charToString(char variable){
    return string_from_format("%c",variable);
}

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
	char* cadena = charToString((char)variable);
	if (esVariableDeclarada(head, cadena)) {
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
	printf("\n\n\n\n POSICION %d \n\n\n\ ",direccion); //fixme borrar cuando todo ande mas o menos bien
	int pagina = (int)(direccion/tamanioPaginas) + paginasCodigo; //Agrego el desplazamiento por las paginas ya ocupadas por el codigo
	int offset = direccion % tamanioPaginas;
	int size=sizeof(int);

	enviar_solicitud(pagina,offset,size);
}

bool existeLabel(t_nombre_etiqueta etiqueta) {
	return dictionary_has_key(pcbActual->indice_etiquetas, etiqueta);
}

t_puntero_instruccion obtenerPosicionLabel(t_nombre_etiqueta etiqueta){
	return (t_puntero_instruccion)dictionary_get(pcbActual->indice_etiquetas, etiqueta);   //al parecer esto anda. Posta que viendolo no me cierra como, pero anda :D
}
