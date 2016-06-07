/*
 * conexiones.c
 *  Created on: 7/6/2016
 *      Author: utnso
 */

#include "conexiones.h"

// ***** Funciones de conexiones ***** //

int getHandshake(int cli) {
	char* handshake = recv_nowait_ws(cli, 1);
	return charToInt(handshake);
}

void warnDebug() {
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_info(activeLogger,
			"Para ingresar manualmente un archivo: Cambiar true por false en cpu.c -> #define DEBUG_IGNORE_UMC, y despues recompilar.");
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
}

void conectar_nucleo() {
	direccionNucleo = crearDireccionParaCliente(config.puertoNucleo,
			config.ipNucleo);
	cliente_nucleo = socket_w();
	connect_w(cliente_nucleo, &direccionNucleo); //conecto cpu a la direccion 'direccionNucleo'
	log_info(activeLogger, "Conectado a Nucleo!");
}

void hacer_handshake_nucleo() {
	char* hand = string_from_format("%c%c", HeaderHandshake, SOYCPU);
	send_w(cliente_nucleo, hand, 2);

	if (getHandshake(cliente_nucleo) != SOYNUCLEO) {
		perror("Se esperaba que CPU se conecte con Nucleo.");
	} else {
		log_info(bgLogger, "Exito al hacer handshake con Nucleo.");
	}
}

void conectar_umc() {
	direccionUmc = crearDireccionParaCliente(config.puertoUMC, config.ipUMC);
	cliente_umc = socket_w();
	connect_w(cliente_umc, &direccionUmc); //conecto umc a la direccion 'direccionUmc'
	log_info(activeLogger, "Conectado a UMC!");
}

void hacer_handshake_umc() {
	char *hand = string_from_format("%c%c", HeaderHandshake, SOYCPU);
	send_w(cliente_umc, hand, 2);

	if (getHandshake(cliente_umc) != SOYUMC) {
		perror("Se esperaba que CPU se conecte con UMC.");
	} else {
		log_info(bgLogger, "Exito al hacer handshake con UMC.");
	}
}

void establecerConexionConUMC() {
	if (!config.DEBUG_IGNORE_UMC) {
		conectar_umc();
		hacer_handshake_umc();
	} else {
		warnDebug();
	}
}
void establecerConexionConNucleo() {
	conectar_nucleo();
	hacer_handshake_nucleo();
}
