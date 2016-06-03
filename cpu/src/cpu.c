/*
 * cpu.c
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "cpu.h"

/*----- Operaciones sobre el PC y avisos por quantum -----*/
void informarInstruccionTerminada() {
	// Le aviso a nucleo que termino una instruccion, para que calcule cuanto quantum le queda al proceso ansisop.
	enviarHeader(cliente_nucleo,headerTermineInstruccion);
	// Acá nucleo tiene que mandarme el header que corresponda, segun si tengo que seguir ejecutando instrucciones o tengo que desalojar.
}
void setearPC(t_PCB* pcb, int pc) {
	log_info(activeLogger, "Actualizando PC de |%d| a |%d|.", pcb->PC, pc);
	pcb->PC = pc;
}
void incrementarPC(t_PCB* pcb) {
	setearPC(pcb, (pcb->PC) + 1);
}

void instruccionTerminada(char* instr) {
	log_debug(activeLogger, "La instruccion |%s| finalizó OK.", instr);
}
void desalojarProceso() {
	char* pcb = NULL;
	int size = serializar_PCB(pcb, pcbActual);
	send_w(cliente_nucleo, pcb, size); //Envio a nucleo el PCB con el PC actualizado.
	// Nucleo no puede hacer pbc->pc+=quantum porque el quantum puede variar en tiempo de ejecución.

	free(pcb);
}

t_pedido maximo(t_pedido pedido1, t_pedido pedido2) { //Determina que pedido está mas lejos respecto del inicio!
	if (pedido1.pagina > pedido2.pagina) {
		return pedido1;
	}
	if (pedido1.pagina == pedido2.pagina) {
		return (pedido1.offset > pedido2.offset ? pedido1 : pedido2);
	}
	return pedido2;
}

/*--------FUNCIONES----------*/

void parsear(char* const sentencia) {
	log_info(activeLogger, "Ejecutando la sentencia |%s|...", sentencia);
	analizadorLinea(sentencia, &funciones, &funcionesKernel);
}

/*--------Funciones----------*/

void esperar_programas() {
	log_debug(bgLogger, "Esperando programas de nucleo.");
	char* header;
	if (config.DEBUG_NO_PROGRAMS) {
		log_debug(activeLogger,
				"DEBUG NO PROGRAMS activado! Ignorando programas...");
	}
	while (!terminar && !config.DEBUG_NO_PROGRAMS) {	//mientras no tenga que terminar
		header = recv_waitall_ws(cliente_nucleo, 1);
		procesarHeader(header);
		free(header);
	}
	log_debug(bgLogger, "Ya no se esperan programas de nucleo.");
}

bool esExcepcion(char* cad){
	return charToInt(cad) == HeaderExcepcion;
}

void procesarHeader(char *header) {

	log_debug(bgLogger, "Llego un mensaje con header %d.", charToInt(header));

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error.");
		break;

	case HeaderHandshake:
		log_error(activeLogger,
				"Segunda vez que se recibe un headerHandshake acá!.");
		exit(EXIT_FAILURE);
		break;

	case HeaderPCB:
		obtenerPCB(); //inicio el proceso de aumentar el PC, pedir a UMC sentencia...
		break;

	case HeaderSentencia:
		obtener_y_parsear();
		break;

	case HeaderDesalojarProceso:
		desalojarProceso();
		break;

	case HeaderExcepcion:
		lanzar_excepcion();
		break;

	default:
		log_error(activeLogger, "Llego cualquier cosa.");
		log_error(activeLogger,
				"Llego el header numero |%d| y no hay una accion definida para el.",
				charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

void pedir_tamanio_paginas() {
	if (!config.DEBUG_IGNORE_UMC) {

		enviarHeader(cliente_umc,HeaderTamanioPagina); //le pido a umc el tamanio de las paginas
		char* tamanio = recv_nowait_ws(cliente_umc, sizeof(int)); //recibo el tamanio de las paginas

		tamanioPaginas = char4ToInt(tamanio);
		log_debug(activeLogger, "El tamaño de paginas es: |%d|",tamanioPaginas);
		free(tamanio);

	} else {
		tamanioPaginas = -1;
		log_debug(activeLogger,
				"UMC DEBUG ACTIVADO! tamanioPaginas va a valer -1.");
	}
}


int longitud_sentencia(t_sentencia* sentencia) {
	return sentencia->offset_fin - sentencia->offset_inicio;
}

int obtener_offset_relativo(t_sentencia* fuente, t_sentencia* destino) {
	int offsetInicio = fuente->offset_inicio;
	int numeroPagina = (int) (offsetInicio / tamanioPaginas); //obtengo el numero de pagina
	int offsetRelativo = (int) offsetInicio % tamanioPaginas; //obtengo el offset relativo

	int longitud = longitud_sentencia(fuente);

	destino->offset_inicio = offsetRelativo;
	destino->offset_fin = offsetRelativo + longitud;

	return numeroPagina;
}

int cantidad_paginas_ocupa(t_sentencia* sentencia) { //precondicion: el offset debe ser el relativo
	int cant = (int) longitud_sentencia(sentencia) / tamanioPaginas;
	return cant + 1;
}

int queda_espacio_en_pagina(t_sentencia* sentencia) { //precondicion: el offset debe ser el relativo
	int longitud = longitud_sentencia(sentencia);
	int desp = sentencia->offset_inicio + longitud;
	return tamanioPaginas - desp;;
}

void enviar_solicitud(int pagina, int offset, int size) {
	t_pedido pedido;
	pedido.offset = offset;
	pedido.pagina = pagina;
	pedido.size = size;

	char* solicitud = string_new();
	int tamanio = serializar_pedido(solicitud, &pedido);

	send_w(cliente_umc, solicitud, tamanio);
	free(solicitud);
}

void pedir_sentencia() {	//pedir al UMC la proxima sentencia a ejecutar
	int entrada = pcbActual->PC; //obtengo la entrada de la instruccion a ejecutar

	t_sentencia* sentenciaActual = list_get(pcbActual->indice_codigo, entrada);	//obtengo el offset de la sentencia
	t_sentencia* sentenciaAux = malloc(sizeof(t_sentencia));

	int pagina = obtener_offset_relativo(sentenciaActual, sentenciaAux);//obtengo el offset relativo

	enviarHeader(cliente_umc,HeaderSolicitudSentencia); //envio el header

	int i = 0;
	int longitud_restante = longitud_sentencia(sentenciaAux); //longitud total de la sentencia
	int cantidad_pags = cantidad_paginas_ocupa(sentenciaAux);

	log_debug(bgLogger, "La instruccion ocupa |%d| paginas", cantidad_pags);

	char* cantRecvs = intToChar(cantidad_pags);
	send_w(cliente_umc,cantRecvs, strlen(cantRecvs));		//envio a umc cuantos recvs tiene que hacer
	free(cantRecvs);

	while (i < cantidad_pags) {				//me fijo si ocupa mas de una pagina
											//notar que cuando paso de una pagina a otra, pierdo una unidad del size total
		log_debug(bgLogger, "envie la pag: |%d|", pagina + i);

		if (longitud_restante >= tamanioPaginas) {//si me paso de la pagina, acorto el offset fin

			longitud_restante = longitud_restante - tamanioPaginas;
			sentenciaAux->offset_fin = tamanioPaginas;

			enviar_solicitud(pagina, sentenciaAux->offset_inicio,
					longitud_sentencia(sentenciaAux));

			sentenciaAux->offset_inicio = 0;//como es contiguo, al pasar a la otra pagina, pongo en 0

		} else {			//si no me paso, sigo igual y terminaria el while
			sentenciaAux->offset_fin = longitud_restante;

			enviar_solicitud(pagina, sentenciaAux->offset_inicio,
					longitud_sentencia(sentenciaAux));
		}

		i++;
	}

	free(sentenciaActual);
	free(sentenciaAux);
}

void esperar_sentencia() {
	log_info(activeLogger, "Esperando sentencia de UMC...");
	char* header = recv_waitall_ws(cliente_umc, sizeof(char));

	log_info(activeLogger, "Sentencia de UMC recibida...");
	procesarHeader(header);


	free(header);
}

void obtenerPCB() {		//recibo el pcb que me manda nucleo
	char* pcb = recv_waitall_ws(cliente_nucleo, sizeof(t_PCB));
	deserializar_PCB(pcbActual, pcb);//reemplazo en el pcb actual de cpu que tiene como variable global

	stack = pcbActual->SP;

	free(pcb);
	pedir_sentencia();
	esperar_sentencia();
}

void enviarPCB() {
	char* pcb = string_new();
	serializar_PCB(pcb, pcbActual);

	send_w(cliente_nucleo, pcb, sizeof(t_PCB));
	free(pcb);
}

void obtener_y_parsear() {
	char* tamanioSentencia = recv_waitall_ws(cliente_umc, sizeof(int));
	int tamanio = char4ToInt(tamanioSentencia);
	free(tamanioSentencia);

	char* sentencia = recv_waitall_ws(cliente_umc, tamanio);
	parsear(sentencia);
	free(sentencia);
}


void finalizar_proceso(){ //voy a esta funcion cuando ejecuto la ultima instruccion o hay una excepcion

	enviarHeader(cliente_nucleo, HeaderTerminoProceso);
	enviarPCB();		//nucleo deberia recibir el PCB para elminar las estructuras
}

void lanzar_excepcion(){
	log_info(activeLogger,"Recibi un mensaje de excepcion :( ");
	log_info(activeLogger,"Terminando la ejecucion del programa actual...");

	finalizar_proceso();

	log_info(activeLogger,"Proceso terminado");
	esperar_programas();
}

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

// ***** Funciones de inicializacion y finalizacion ***** //
void cargarConfig() {
	t_config* configCPU;
	configCPU = config_create("cpu.cfg");
	config.puertoNucleo = config_get_int_value(configCPU, "PUERTO_NUCLEO");
	config.ipNucleo = config_get_string_value(configCPU, "IP_NUCLEO");
	config.puertoUMC = config_get_int_value(configCPU, "PUERTO_UMC");
	config.ipUMC = config_get_string_value(configCPU, "IP_UMC");
	config.DEBUG_IGNORE_UMC = config_get_int_value(configCPU, "DEBUG_IGNORE_UMC");
	config.DEBUG_NO_PROGRAMS = config_get_int_value(configCPU, "DEBUG_NO_PROGRAMS");
	config.DEBUG_RAISE_LOG_LEVEL = config_get_int_value(configCPU, "DEBUG_RAISE_LOG_LEVEL");
}
void inicializar() {
	cargarConfig();
	pcbActual = malloc(sizeof(t_PCB));
	crearLogs(string_from_format("cpu_%d", getpid()), "CPU", config.DEBUG_RAISE_LOG_LEVEL);
	log_info(activeLogger, "Soy CPU de process ID %d.", getpid());
	inicializar_primitivas();
}
void finalizar() {

	log_info(activeLogger,"Finalizando proceso cpu");

	if (!config.DEBUG_NO_PROGRAMS) {
		log_debug(activeLogger, "Destruyendo pcb...");
		pcb_destroy(pcbActual);
		log_debug(activeLogger, "PCB Destruido...");
	}
	liberar_primitivas();
	destruirLogs();

	close(cliente_nucleo);
	close(cliente_umc);
	exit(EXIT_SUCCESS);

	log_info(activeLogger,"Proceso cpu finalizado");
}

/*------------otras------------*/
void handler(int sign) {
	if (sign == SIGUSR1) {
		log_info(activeLogger, "Recibi SIGUSR1! Adios a todos!");
		terminar = true; //Setea el flag para que termine CPU al terminar de ejecutar la instruccio
		log_info(activeLogger, "Esperando a que termine la ejecucion del programa actual...");
	} else {
		log_info(activeLogger,
				"Recibi la señal numero |%d|, que no es SIGUSR1, asi que me quedo :)", sign);
	}
}

int main() {

	signal(SIGUSR1,handler);
	inicializar();

	//conectarse a umc
	establecerConexionConUMC();

	//conectarse a nucleo
	establecerConexionConNucleo();

	pedir_tamanio_paginas();

	//CPU se pone a esperar que nucleo le envie PCB
	esperar_programas();

	finalizar();
	return EXIT_SUCCESS;
}
