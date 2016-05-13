/*
 * header.h
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#ifndef OTROS_HEADER_H_
#define OTROS_HEADER_H_

// Documentacion de los headers:
// https://github.com/sisoputnfrba/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/wiki/protocolo

typedef enum {
	HeaderError,
	HeaderScript,
	HeaderHandshake,
	HeaderImprimirVariableNucleo,
	HeaderImprimirTextoNucleo,
	HeaderImprimirVariableConsola,
	HeaderImprimirTextoConsola,
	HeaderConsolaFinalizarNormalmente,
	HeaderPCB,
	HeaderSolicitudSentencia,
	HeaderSentencia,
	HeaderPedirValorVariable,
	HeaderPedirValorVariableCompartida,
	HeaderAsignarValor,
	HeaderConsolaFinalizarRechazado,
	HeaderPedirPagina,
	HeaderLiberarRecursosPagina,
	HeaderGrabarPagina,
	HeaderReservarEspacio,
	HeaderPedirContenidoPagina,
	HeaderErrorNoHayPaginas,
	HeaderTeReservePagina,
	HeaderNoExistePagina,
	HeaderNoExisteTablaDePag
} header_t;

char* headerToMSG(header_t header);

#endif /* OTROS_HEADER_H_ */
