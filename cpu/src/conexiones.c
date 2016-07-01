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
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
}

void conectar_nucleo() {
	direccionNucleo = crearDireccionParaCliente(config.puertoNucleo,
			config.ipNucleo);
	nucleo = socket_w();
	connect_w(nucleo, &direccionNucleo); //conecto cpu a la direccion 'direccionNucleo'
	log_info(activeLogger, "Conectado a Nucleo!");
}

void hacer_handshake_nucleo() {
	char* hand = string_from_format("%c%c", HeaderHandshake, SOYCPU);
	send_w(nucleo, hand, 2);

	if (getHandshake(nucleo) != SOYNUCLEO) {
		perror("Se esperaba que CPU se conecte con Nucleo.");
	} else {
		log_info(bgLogger, "Exito al hacer handshake con Nucleo.");
	}
}

void conectar_umc() {
	direccionUmc = crearDireccionParaCliente(config.puertoUMC, config.ipUMC);
	umc = socket_w();
	connect_w(umc, &direccionUmc); //conecto umc a la direccion 'direccionUmc'
	log_info(activeLogger, "Conectado a UMC!");
}

void hacer_handshake_umc() {
	char *hand = string_from_format("%c%c", HeaderHandshake, SOYCPU);
	send_w(umc, hand, 2);

	if (getHandshake(umc) != SOYUMC) {
		perror("Se esperaba que CPU se conecte con UMC.");
	} else {
		log_info(bgLogger, "Exito al hacer handshake con UMC.");
	}
}

void establecerConexionConUMC() {
	if (!config.DEBUG_IGNORE_UMC) {
		conectar_umc();
		log_info(activeLogger,"Estoy handshakeando");
		hacer_handshake_umc();
	} else {
		warnDebug();
	}

	pedir_tamanio_paginas();
}
void establecerConexionConNucleo() {
	if(!config.DEBUG_IGNORE_NUCLEO){
		conectar_nucleo();
		hacer_handshake_nucleo();
	}else{
		warnDebug();
	}
}
