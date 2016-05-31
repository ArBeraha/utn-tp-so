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
	HeaderTamanioPagina,
	HeaderPCB,
	HeaderSolicitudSentencia,
	HeaderSentencia,
	HeaderPedirValorVariable,
	HeaderPedirValorVariableCompartida,
	HeaderAsignarValor,
	HeaderAsignarValorVariableCompartida,
	HeaderAsigneValorVariableCompartida, // Nucleo -> CPU, tras asignar un valor a una var compartida.
	HeaderConsolaFinalizarRechazado,
	HeaderPedirPagina,
	HeaderLiberarRecursosPagina,
	HeaderGrabarPagina,
	HeaderReservarEspacio,
	HeaderPedirContenidoPagina,
	HeaderErrorNoHayPaginas,
	HeaderTeReservePagina,
	HeaderNoExistePagina,
	HeaderNoExisteTablaDePag,
	HeaderSignal,
	HeaderWait,
	headerTermineInstruccion,
	HeaderDesalojarProceso,
	HeaderConsultaEspacioSwap,
	//Para Swap
	HeaderOperacionIniciarProceso,
	HeaderOperacionLectura,
	HeaderOperacionEscritura,
	HeaderOperacionFinalizarProceso,
	HeaderErrorParaIniciar,
	HeaderProcesoAgregado,
	HeaderNoHayEspacio,
	HeaderHayQueCompactar,
	HeaderLecturaCorrecta,
	HeaderLecturaErronea,
	HeaderProcesoEliminado,
	HeaderProcesoNOEliminado,
	HeaderEscrituraCorrecta,
	HeaderEscrituraErronea
} header_t;

char* headerToMSG(header_t header);

#endif /* OTROS_HEADER_H_ */
