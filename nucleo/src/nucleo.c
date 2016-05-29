/*
 * nucleo.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */
#include "nucleo.h"

/* ---------- INICIO PARA UMC ---------- */
bool pedirPaginas(int PID, char* codigo) {
	t_proceso* proceso = (t_proceso*) PID;
	char* serialPaginas = intToChar4(proceso->PCB->cantidad_paginas);
	char* serialPid = intToChar4(PID);
	bool hayMemDisponible = false;
	char respuesta;
	if (DEBUG_IGNORE_UMC || DEBUG_IGNORE_UMC_PAGES) { // Para DEBUG
		log_warning(activeLogger,
				"DEBUG_IGNORE_UMC_PAGES está en true! Se supone que no hay paginas");
	} else { /* Para curso normal del programa
	 Estos mutex garantizan que por ejemplo no haga cada hilo un send (esto podria solucionarse tambien juntando los send, pero es innecesario porque si o si hay que sincronizar)
	 y que no se van a correr los sends de un hilo 1, los del hilo 2, umc responde por hilo 1 (lo primero que le llego) y como corre hilo 2, esa respuesta llega al hilo 2 en vez de al 1. */
		pthread_mutex_lock(&mutexUMC);
		enviarHeader(umc,HeaderScript);
		send_w(umc, serialPid, sizeof(int));
		send_w(umc, serialPaginas, sizeof(int));
		enviarLargoYMensaje(umc, codigo);
		read(umc, &respuesta, 1);
		pthread_mutex_unlock(&mutexUMC);
		hayMemDisponible = (bool) ((int) respuesta);
		if (hayMemDisponible == true)
			log_debug(bgLogger, "Hay memoria disponible para el proceso %d.",
					PID);
		else if (hayMemDisponible == false)
			log_debug(bgLogger, "No hay memoria disponible para el proceso %d.",
					PID);
		else
			log_warning(activeLogger, "Umc debería enviar (0 o 1) y envió %d",
					hayMemDisponible);
	}
	free(serialPid);
	free(serialPaginas);
	return hayMemDisponible;
}
int getHandshake() {
	char* handshake = recv_nowait_ws(umc, 1);
	return charToInt(handshake);
}
void handshakearUMC() {
	char *hand = string_from_format("%c%c", HeaderHandshake, SOYNUCLEO);
	send_w(umc, hand, 2);

	log_debug(bgLogger, "UMC handshakeo.");
	if (getHandshake() != SOYUMC) {
		perror("Se esperaba conectarse a la UMC.");
	} else
		log_debug(bgLogger, "Núcleo recibió handshake de UMC.");
}
void establecerConexionConUMC() {
	direccionUMC = crearDireccionParaCliente(config.puertoUMC,
			config.ipUMC);
	umc = socket_w();
	connect_w(umc, &direccionUMC);
}
void warnDebug() {
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---");
	log_info(activeLogger, "NO SE ESTABLECE CONEXION CON UMC EN ESTE MODO!");
	log_info(activeLogger,
			"Para correr nucleo en modo normal, settear en false el define DEBUG_IGNORE_UMC.");
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---");
}
void conectarAUMC() {
	if (!DEBUG_IGNORE_UMC) {
		log_debug(bgLogger, "Iniciando conexion con UMC...");
		establecerConexionConUMC();
		log_info(activeLogger, "Conexion a la UMC correcta :).");
		handshakearUMC();
		log_info(activeLogger, "Handshake con UMC finalizado exitosamente.");
	} else {
		warnDebug();
	}
}
/*  ----------INICIO CONSOLA ---------- */
char* getScript(int consola) {
	log_debug(bgLogger, "Recibiendo archivo de consola %d...", consola);
	char* script = leerLargoYMensaje(consola);
	log_info(activeLogger, "Script de consola %d recibido:\n%s", consola,
			script);
	clientes[consola].atentido = false; //En true se bloquean, incluso si mando muchos de una consola usando un FOR para mandar el comando (leer wikia)
	return script;
}
/*  ----------INICIO NUCLEO ---------- */
void cargarCFG() {
	t_config* configNucleo;
	configNucleo = config_create("nucleo.cfg");
	config.puertoConsola = config_get_int_value(configNucleo, "PUERTO_PROG");
	config.puertoCPU = config_get_int_value(configNucleo, "PUERTO_CPU");
	config.quantum = config_get_int_value(configNucleo, "QUANTUM");
	config.queantum_sleep = config_get_int_value(configNucleo, "QUANTUM_SLEEP");
	config.sem_ids = config_get_array_value(configNucleo, "SEM_ID");
	config.semInit = config_get_array_value(configNucleo, "SEM_INIT");
	config.io_ids = config_get_array_value(configNucleo, "IO_ID");
	config.ioSleep = config_get_array_value(configNucleo, "IO_SLEEP");
	config.sharedVars = config_get_array_value(configNucleo, "SHARED_VARS");
	config.ipUMC = string_duplicate(config_get_string_value(configNucleo, "IP_UMC"));
	config.puertoUMC = config_get_int_value(configNucleo, "PUERTO_UMC");

	// Cargamos los IO
	int i = 0;
	while (config.io_ids[i] != '\0') {
		t_IO* io = malloc(sizeof(t_IO));
		io->retardo = atoi(config.ioSleep[i]);
		io->cola = queue_create();
		io->estado = INACTIVE;
		dictionary_put(tablaIO, config.io_ids[i], io);
		//printf("ID:%s SLEEP:%d\n",config.io_ids[i],io->retardo);
		i++;
	}
	// Cargamos los SEM
	i = 0;
	while (config.sem_ids[i] != '\0') {
		t_semaforo* sem = malloc(sizeof(t_semaforo));
		sem->valor = atoi(config.semInit[i]);
		sem->cola = queue_create();
		dictionary_put(tablaSEM, config.sem_ids[i], sem);
		//printf("SEM:%s INIT:%d\n",config.sem_ids[i],*init);
		i++;
	}
	// Cargamos las variables compartidas
	i = 0;
	while (config.sharedVars[i] != '\0') {
		int* init = malloc(sizeof(int));
		*init = 0;
		dictionary_put(tablaSEM, config.sharedVars[i], init);
		//printf("SHARED:%s VALUE:%d\n",config.sharedVars[i],*init);
		i++;
	}
	config_destroy(configNucleo);
}
void configHilos() {
	pthread_attr_init(&detachedAttr);
	pthread_attr_setdetachstate(&detachedAttr, PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&mutexProcesos, NULL);
	pthread_mutex_init(&mutexUMC, NULL);
}
struct timeval newEspera() {
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}
void inicializar(){
	listaProcesos = list_create();
	colaCPU = queue_create();
	colaListos = queue_create();
	colaSalida = queue_create();
	tablaIO = dictionary_create();
	tablaSEM = dictionary_create();
	tablaGlobales = dictionary_create();
	cargarCFG();
	configHilos();
	algoritmo = FIFO;
}
void finalizar() {
	destruirLogs();
	list_destroy(listaProcesos);
	queue_destroy(colaCPU);
	queue_destroy(colaListos);
	queue_destroy(colaSalida);
	pthread_mutex_destroy(&mutexProcesos);
	pthread_mutex_destroy(&mutexUMC);
	pthread_attr_destroy(&detachedAttr);
	destruirSemaforos();
	destruirIOs();
	destruirCompartidas();
}
void destruirSemaforo(t_semaforo* sem){
	queue_destroy(sem->cola);
	free(sem);
}
void destruirSemaforos(){
	dictionary_destroy_and_destroy_elements(tablaSEM, (void*)destruirSemaforo);
}
void destruirIO(t_IO* io){
	queue_destroy(io->cola);
	free(io);
}
void destruirIOs(){
	dictionary_destroy_and_destroy_elements(tablaIO, (void*)destruirIO);
}
void destruirCompartida(int* compartida){
	free(compartida);
}
void destruirCompartidas(){
	dictionary_destroy_and_destroy_elements(tablaGlobales, (void*)destruirSemaforo);
}
void procesarHeader(int cliente, char *header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));
	clientes[cliente].atentido = true;

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger, "Llego un handshake");
		payload_size = 1;
		payload = malloc(payload_size);
		read(clientes[cliente].socket, payload, payload_size);
		log_debug(bgLogger, "Llego un mensaje con payload %d",
				charToInt(payload));
		if ((charToInt(payload) == SOYCONSOLA)
				|| (charToInt(payload) == SOYCPU)) {
			log_debug(bgLogger,
					"Es un cliente apropiado! Respondiendo handshake");
			clientes[cliente].identidad = charToInt(payload);
			enviarHeader(clientes[cliente].socket, SOYNUCLEO);
		} else {
			log_error(activeLogger,
					"No es un cliente apropiado! rechazada la conexion");
			log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
			quitarCliente(cliente);
		}
		free(payload);
		clientes[cliente].atentido = false;
		break;

	case HeaderImprimirVariableNucleo:
		imprimirVariable(cliente);
		break;

	case HeaderImprimirTextoNucleo:
		imprimirTexto(cliente);
		break;

	case HeaderScript:
		cargarProceso(cliente);
		break;

	case HeaderPedirValorVariableCompartida:
		devolverCompartida(cliente);
		break;

	case HeaderAsignarValorVariableCompartida:
		asignarCompartida(cliente);
		break;

	case HeaderSignal:
		signalSemaforo(cliente);
		break;

	case HeaderWait:
		waitSemaforo(cliente);
		break;

	default:
		log_error(activeLogger, "Llego cualquier cosa.");
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
		quitarCliente(cliente);
		break;
	}
}
int main(void) {
	system("clear");
	int i;
	struct timeval espera = newEspera(); // Periodo maximo de espera del select
	char header[1];
	crearLogs("Nucleo", "Nucleo");
	inicializar();
	testear(test_serializacion);
	testear(test_nucleo);
	//system("clear");
	inicializar();
	configurarServidorExtendido(&socketConsola, &direccionConsola,
			config.puertoConsola, &tamanioDireccionConsola, &activadoConsola);
	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puertoCPU,
			&tamanioDireccionCPU, &activadoCPU);

	inicializarClientes();

	log_info(activeLogger, "Esperando conexiones ...");
	conectarAUMC();

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketConsola, &socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);

		mayorDescriptor =
				(socketConsola > socketCPU) ? socketConsola : socketCPU;
		incorporarClientes();

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, &espera);

		if (tieneLectura(socketConsola))
			procesarNuevasConexionesExtendido(&socketConsola);

		if (tieneLectura(socketCPU))
			procesarNuevasConexionesExtendido(&socketCPU);

		for (i = 0; i < getMaxClients(); i++) {
			if (tieneLectura(clientes[i].socket)) {
				if (read(clientes[i].socket, header, 1) == 0) {
					log_error(activeLogger, "Un cliente se desconectó.");
					quitarCliente(i);
				} else
					procesarHeader(i, header);
			}
		}
		planificarProcesos();
	}
	finalizar();
	return EXIT_SUCCESS;
}
