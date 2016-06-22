/*
 * header.c
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#include "header.h"
#include <commons/string.h>

char* headerToMSG(header_t header)
{
	return string_from_format("%c",header);
}

char* headerToString(header_t header) {
	// Para actualizar usar la lista de headers de header.h y usar expresiones regulares
	// Buscando: 			Header([A-Za-z0-9]+), //\d*
	// Reemplazando por:	case Header\1 : return "Header\1";
	switch (header) {
		case HeaderError: return "HeaderError";
		case HeaderScript: return "HeaderScript";
		case HeaderHandshake: return "HeaderHandshake";
		case HeaderImprimirVariableNucleo: return "HeaderImprimirVariableNucleo";
		case HeaderImprimirTextoNucleo: return "HeaderImprimirTextoNucleo";
		case HeaderImprimirVariableConsola: return "HeaderImprimirVariableConsola";
		case HeaderImprimirTextoConsola: return "HeaderImprimirTextoConsola";
		case HeaderConsolaFinalizarNormalmente: return "HeaderConsolaFinalizarNormalmente";
		case HeaderTamanioPagina: return "HeaderTamanioPagina";
		case HeaderPCB: return "HeaderPCB";
		case HeaderSolicitudSentencia: return "HeaderSolicitudSentencia";
		case HeaderSentencia: return "HeaderSentencia";
		case HeaderPedirValorVariable: return "HeaderPedirValorVariable";
		case HeaderPedirValorVariableCompartida: return "HeaderPedirValorVariableCompartida";
		case HeaderAsignarValor: return "HeaderAsignarValor";
		case HeaderAsignarValorVariableCompartida: return "HeaderAsignarValorVariableCompartida";
		case HeaderAsigneValorVariableCompartida: return "HeaderAsigneValorVariableCompartida";
		case HeaderConsolaFinalizarRechazado: return "HeaderConsolaFinalizarRechazado";
		case HeaderPedirPagina: return "HeaderPedirPagina";
		case HeaderLiberarRecursosPagina: return "HeaderLiberarRecursosPagina";
		case HeaderGrabarPagina: return "HeaderGrabarPagina";
		case HeaderReservarEspacio: return "HeaderReservarEspacio";
		case HeaderPedirContenidoPagina: return "HeaderPedirContenidoPagina";
		case HeaderErrorNoHayPaginas: return "HeaderErrorNoHayPaginas";
		case HeaderTeReservePagina: return "HeaderTeReservePagina";
		case HeaderNoExistePagina: return "HeaderNoExistePagina";
		case HeaderNoExisteTablaDePag: return "HeaderNoExisteTablaDePag";
		case HeaderEntradaSalida: return "HeaderEntradaSalida";
		case HeaderSignal: return "HeaderSignal";
		case HeaderWait: return "HeaderWait";
		case HeaderExcepcion: return "HeaderExcepcion";
		//case HeaderTermineInstruccion: return "HeaderTermineInstruccion";
		case HeaderDesalojarProceso: return "HeaderDesalojarProceso";
		case HeaderTerminoProceso: return "HeaderTerminoProceso";
		case HeaderContinuarProceso: return "HeaderContinuarProceso";
		case HeaderConsultaEspacioSwap: return "HeaderConsultaEspacioSwap";
		case HeaderOperacionIniciarProceso: return "HeaderOperacionIniciarProceso";
		case HeaderOperacionLectura: return "HeaderOperacionLectura";
		case HeaderOperacionEscritura: return "HeaderOperacionEscritura";
		case HeaderOperacionFinalizarProceso: return "HeaderOperacionFinalizarProceso";
		case HeaderErrorParaIniciar: return "HeaderErrorParaIniciar";
		case HeaderProcesoAgregado: return "HeaderProcesoAgregado";
		case HeaderNoHayEspacio: return "HeaderNoHayEspacio";
		case HeaderHayQueCompactar: return "HeaderHayQueCompactar";
		case HeaderLecturaCorrecta: return "HeaderLecturaCorrecta";
		case HeaderLecturaErronea: return "HeaderLecturaErronea";
		case HeaderProcesoEliminado: return "HeaderProcesoEliminado";
		case HeaderProcesoNOEliminado: return "HeaderProcesoNOEliminado";
		case HeaderEscrituraCorrecta: return "HeaderEscrituraCorrecta";
		case HeaderEscrituraErronea: return "HeaderEscrituraErronea";
		case HeaderProcesoNoEncontrado: return "HeaderProcesoNoEncontrado";
		case HeaderPID: return "HeaderPID";
	}
}
