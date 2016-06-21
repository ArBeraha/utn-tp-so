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
	HeaderError, //0
	HeaderScript, //1
	HeaderHandshake, //2
	HeaderImprimirVariableNucleo, //3
	HeaderImprimirTextoNucleo, //4
	HeaderImprimirVariableConsola, //5
	HeaderImprimirTextoConsola, //6
	HeaderConsolaFinalizarNormalmente, //7
	HeaderTamanioPagina, //8
	HeaderPCB, //9
	HeaderSolicitudSentencia, //10
	HeaderSentencia, //11
	HeaderPedirValorVariable, //12
	HeaderPedirValorVariableCompartida, //13
	HeaderAsignarValor, //14
	HeaderAsignarValorVariableCompartida, //15
	HeaderAsigneValorVariableCompartida, //16
	HeaderConsolaFinalizarRechazado, //17
	HeaderPedirPagina, //18
	HeaderLiberarRecursosPagina, //19
	HeaderGrabarPagina, //20
	HeaderReservarEspacio, //21
	HeaderPedirContenidoPagina, //22
	HeaderErrorNoHayPaginas, //23
	HeaderTeReservePagina, //24
	HeaderNoExistePagina, //25
	HeaderNoExisteTablaDePag, //26
	HeaderEntradaSalida, //27
	HeaderSignal, //28
	HeaderWait, //29
	HeaderExcepcion, //30
	headerTermineInstruccion, //31
	HeaderDesalojarProceso, //32
	HeaderTerminoProceso, //33
	HeaderContinuarProceso, //34
	HeaderConsultaEspacioSwap, //35
	//Para Swap
	HeaderOperacionIniciarProceso, //36
	HeaderOperacionLectura, //37
	HeaderOperacionEscritura, //38
	HeaderOperacionFinalizarProceso, //39
	HeaderErrorParaIniciar, //40
	HeaderProcesoAgregado, //41
	HeaderNoHayEspacio, //42
	HeaderHayQueCompactar, //43
	HeaderLecturaCorrecta, //44
	HeaderLecturaErronea, //45
	HeaderProcesoEliminado, //46
	HeaderProcesoNOEliminado, //47
	HeaderEscrituraCorrecta, //48
	HeaderEscrituraErronea, //49
	HeaderProcesoNoEncontrado, //50
	HeaderPID
} header_t;

char* headerToMSG(header_t header);

#endif /* OTROS_HEADER_H_ */
