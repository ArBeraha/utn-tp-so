/*
 * cpu.c
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "cpu.h"

bool noEsEnd(char* sentencia){
	return strcmp(sentencia,"end")!=0
			&& strcmp(sentencia,"\tend")!=0
			&& strcmp(sentencia,"\t\tend")!=0;
}

void finalizar_proceso(bool normalmente){ //voy a esta funcion cuando ejecuto la ultima instruccion o hay una excepcion
	if(normalmente){
		log_info(activeLogger,ANSI_COLOR_GREEN "El proceso ansisop ejecutó su última instrucción." ANSI_COLOR_RESET);
	}
	enviarHeader(nucleo, HeaderTerminoProceso);
	enviarHeader(umc, HeaderTerminoProceso);
	pcb_destroy(pcbActual);
	pcbActual=NULL;
}

/*----- Operaciones sobre el PC y avisos por quantum -----*/
void setearPC(t_PCB* pcb, t_puntero_instruccion pc) {
	if(!CU_is_test_running()){
		log_info(activeLogger, "Actualizando PC de |%d| a |%d|.", pcb->PC, (int)pc);
	}
	pcb->PC = (int)pc;
}

void incrementarPC(t_PCB* pcb) {
	setearPC(pcb, (t_puntero_instruccion)((pcb->PC) + 1));
}

void informarInstruccionTerminada(char* sentencia) { /* NO SE LLAMA */

	// Le aviso a nucleo que termino una instruccion, para que calcule cuanto quantum le queda al proceso ansisop.
	if(!noEsEnd(sentencia)){
		finalizar_proceso(true);
	}else{
		enviarHeader(nucleo,headerTermineInstruccion);
		log_debug(debugLogger,"Informé a nucleo del fin de una instrucción");
	}

	// Acá nucleo tiene que mandarme el header que corresponda, segun si tengo que seguir ejecutando instrucciones o tengo que desalojar.
	// Eso se procesa en otro lado, porque la ejeución de instrucciones esta anidada en un while
	// por lo que no tengo que recibir el header aca
}

void loggearFinDePrimitiva(char* primitiva) {
	log_debug(debugLogger, ANSI_COLOR_GREEN "La primitiva |%s| finalizó OK." ANSI_COLOR_RESET, primitiva);
}

void desalojarProceso() {
	log_info(activeLogger, "Desalojando proceso...");

	enviarPCB();
	ejecutando = false;
	log_info(activeLogger, "Proceso desalojado.");
}

bool puedo_terminar(){
	return terminar && !ejecutando;
}

/*--------FUNCIONES----------*/
void esperar_programas() {
	log_debug(debugLogger, "Esperando programas de nucleo.");
	char* header;
	while (!puedo_terminar()) {
		header = recv_waitall_ws(nucleo, 1);
		procesarHeader(header);
		free(header);
	}
	log_debug(debugLogger, "Ya no se esperan programas de nucleo.");
}

void procesarHeader(char *header) {

	log_debug(debugLogger, "Llego el header: %s", headerToString(charToInt(header)));

	switch (charToInt(header)) {

	case HeaderError:
		log_error(errorLogger, "Header de Error.");
		break;

	case HeaderHandshake:
		log_error(errorLogger,
				"Segunda vez que se recibe un headerHandshake acá!.");
		exit(EXIT_FAILURE);
		break;

	case HeaderPCB:
		overflow = false;
		obtenerPCB(); //inicio el proceso de aumentar el PC, pedir a UMC sentencia...
		break;

	case HeaderContinuarProceso:
		obtener_y_parsear();
		break;

	case HeaderDesalojarProceso:
		desalojarProceso();
		break;

	case HeaderExcepcion:
		lanzar_excepcion_overflow();
		break;

	default:
		log_error(errorLogger, "Llego cualquier cosa.");
		log_error(errorLogger,
				"Llego el header numero |%d| y no hay una accion definida para el.",
				charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

/**
 * Pido el tamaño de tamaño de pagina a UMC,
 * Si ignoro UMC por config, lo seteo en -99999
 */
void pedir_tamanio_paginas() {
	enviarHeader(umc,HeaderTamanioPagina); //le pido a umc el tamanio de las paginas
	char* tamanio = recv_nowait_ws(umc, sizeof(int)); //recibo el tamanio de las paginas
	tamanioPaginas = char4ToInt(tamanio);
	log_debug(debugLogger, "El tamaño de paginas es: |%d|",tamanioPaginas);
	free(tamanio);
}


int longitud_sentencia(t_sentencia* sentencia) {
	return sentencia->offset_fin - sentencia->offset_inicio; // para la otra interpretacion de esto, sumar 1 aca y listo!
}

/**
 * Recibo el offset absoluto y lo transformo en (numeroPagina, destino->offset_inicio, destino->offset_fin).
 * Ej: (21,40) -> (5,1,20)
 */
int obtener_offset_relativo(t_sentencia* fuente, t_sentencia* destino) {
	int offsetAbsoluto = fuente->offset_inicio;
	int paginaInicio = (int) (offsetAbsoluto / tamanioPaginas); // Pagina donde inicia la sentencia
	int offsetRelativo = offsetAbsoluto % tamanioPaginas; //obtengo el offset relativo
	destino->offset_inicio = offsetRelativo;
	destino->offset_fin = offsetRelativo + longitud_sentencia(fuente);
	return paginaInicio;
}

void sacarSaltoDeLinea(char* texto, int pos)
{
	if(texto[pos-1]=='\n'){
		texto[pos-1]='\0';
	}
}

void recibirFragmentoDeSentencia(int size){
	log_debug(debugLogger, "Recibiendo parte de una sentencia. Tamaño del fragmento: |%d|...", size);
	char* serialSentencia = recv_waitall_ws(umc, size);
	sacarSaltoDeLinea(serialSentencia, size);
	char* sentencia = malloc(size+1);
	sentencia[size]='\0';
	memcpy(sentencia,serialSentencia,size);
	log_debug(debugLogger, "Recibido el fragmento de sentencia |%s|", sentencia);
	string_append(&sentenciaPedida, sentencia);
	free(serialSentencia);
	free(sentencia);
}

/**
 * Envia a UMC: pag, offest y tamaño, es decir, un t_pedido.
 * Chequea que no haya overflow, mas alla de si pide una sentencia o una variable.
 * Se usa para indicar en que posicion escribir, pedir variable y pedir sentencia.
 */
void enviar_solicitud(int pagina, int offset, int size) {
	t_pedido pedido;
	pedido.offset = offset;
	pedido.pagina = pagina;
	pedido.size = size;

	char* solicitud = string_new();

	int tamanio = serializar_pedido(solicitud, &pedido);

	send_w(umc, solicitud, tamanio);

	log_info(activeLogger,"Solicitud enviada: (nPag,offset,size)=(%d,%d,%d)", pagina, offset, size);

	char* stackOverflowFlag = recv_waitall_ws(umc, sizeof(int));
	overflow = !char4ToInt(stackOverflowFlag);
	free(stackOverflowFlag);
	if (overflow) {
		lanzar_excepcion_overflow();
	}
	free(solicitud);
}

t_sentencia* obtener_sentencia_relativa(int* paginaInicioSentencia) {
	//imprimir_PCB(pcbActual);
	t_sentencia* sentenciaAbsoluta = list_get(pcbActual->indice_codigo, pcbActual->PC);	//obtengo el offset de la sentencia
	t_sentencia* sentenciaRelativa = malloc(sizeof(t_sentencia));
	(*paginaInicioSentencia) = obtener_offset_relativo(sentenciaAbsoluta, sentenciaRelativa); //obtengo el offset relativo
	//free(sentenciaAbsoluta); USTEDES SON DIABOLICOS! QUE LE HACEN A MI POBRE PCB?
	return sentenciaRelativa;
}

/**
 * Si no le ponia el _ me daba error :(
 */
int minimo(int a, int b) {
	return a < b ? a : b;
}

/**
 * Segun la longitud restante, determina si la proxima pagina a pedir corresponde
 * completamente a la sentencia que estamos manejando. Si correspondiese solo una parte o fuese
 * toda de otra sentencia, retorna false.
 */
bool paginaCompleta(int longitud_restante) {
	return longitud_restante >= tamanioPaginas;
}

/**
 * Pide una pagina entera a UMC
 */
void pedirPaginaCompleta(int pagina) {
	enviarHeader(umc, HeaderSolicitudSentencia);
	enviar_solicitud(pagina, 0, tamanioPaginas);
	recibirFragmentoDeSentencia(tamanioPaginas);
}

void pedirPrimeraSentencia(t_sentencia* sentenciaRelativa, int pagina, int* longitud_restante) {
	int tamanioPrimeraSentencia = minimo(*longitud_restante,
				tamanioPaginas - sentenciaRelativa->offset_inicio); //llega hasta su final o hasta que se termine la pagina, lo mas pequeño
	enviarHeader(umc, HeaderSolicitudSentencia);
	enviar_solicitud(pagina, sentenciaRelativa->offset_inicio, tamanioPrimeraSentencia);
	(*longitud_restante) -= tamanioPrimeraSentencia;
	recibirFragmentoDeSentencia(tamanioPrimeraSentencia);
}

void pedirUltimaSentencia(t_sentencia* sentenciaRelativa, int pagina, int longitud_restante){
	enviarHeader(umc, HeaderSolicitudSentencia);
	enviar_solicitud(pagina, 0, longitud_restante); //Desde el inicio, con tamaño identico a lo que me falta leer.
	recibirFragmentoDeSentencia(longitud_restante);
}

/**
 * Envia a UMC: cantidad de paginas (cantRecvs) que voy a pedir,
 * t_pedido_1 <---- Setendo el offset de inicio correctamente, y el de fin tambien si terminase en esta pagina.
 * t_pedido_2, ...., t_pedido_n-1 <---- paginas que se piden completas
 * t_pedido_n <---- Si no es la pagina completa, setea el offset fin correcto para no pedir de mas.
 */
void pedirYRecibirSentencia(int* tamanio) {	//pedir al UMC la proxima sentencia a ejecutar
	log_info(activeLogger, "Iniciando pedido de sentencia...");
	int paginaAPedir; // Lo inicializa obtener_sentencia_relativa
	t_sentencia* sentenciaRelativa = obtener_sentencia_relativa(&paginaAPedir);
	int longitud_restante = longitud_sentencia(sentenciaRelativa); //longitud de la sentencia que aun no pido
	(*tamanio) = longitud_restante;

	// Pido la primera pagina, empezando donde corresponde y terminando donde corresponda.
	pedirPrimeraSentencia(sentenciaRelativa, paginaAPedir, &longitud_restante);
	paginaAPedir++;

	//Si falta pedir paginas, pido todas las paginas que correspondan totalmente a la sentencia que quiero
	while (paginaCompleta(longitud_restante)) {
		pedirPaginaCompleta(paginaAPedir);
		longitud_restante -= tamanioPaginas; // Le resto el tamaño de la pagina que pedi, la cual esta completa.
		paginaAPedir++;
	}

	// Si quedase una pagina sin pedir, por no estar completa la ultima pagina, la pido.
	if(longitud_restante>0){
		pedirUltimaSentencia(sentenciaRelativa, paginaAPedir, longitud_restante);
		paginaAPedir++;
	}

	log_info(activeLogger, "Pedido de sentencia finalizado.");
	free(sentenciaRelativa);
}

void enviarPID(){
	enviarHeader(umc,HeaderPID);
	char* pid = intToChar4(pcbActual->PID);
	send_w(umc,pid,sizeof(int));
	free(pid);
}

void recibirCantidadDePaginasDeCodigo(){
	log_debug(debugLogger,"Recibiendo la cantidad de paginas de codigo de UMC.");
	char* pags = recv_waitall_ws(umc,sizeof(int));
	cantidadPaginasCodigo = char4ToInt(pags);
	log_debug(debugLogger,"Recibida la cantidad de paginas de codigo |%d|.", cantidadPaginasCodigo);
	free(pags);
}

void recibir_quantum_sleep(){
	char* quantum = recv_waitall_ws(nucleo, sizeof(int));
	quantum_sleep = char4ToInt(quantum);
	log_debug(debugLogger, "Valor de quantum_sleep: |%d|",quantum_sleep);

	free(quantum);
}

void obtenerPCB() {		//recibo el pcb que me manda nucleo
	if(pcbActual!=NULL){
		pcb_destroy(pcbActual);
	}
	ejecutando = true;

	pcbActual=malloc(sizeof(t_PCB));
	log_debug(debugLogger, "Recibiendo PCB...");
	char* serialPCB = leerLargoYMensaje(nucleo);
	log_debug(debugLogger, "PCB recibido!");
	deserializar_PCB(pcbActual, serialPCB);
	enviarPID();
	cantidadPaginasCodigo = pcbActual->cantidad_paginas;
	stack = pcbActual->SP;
	free(serialPCB);
}

void enviarPCB() {
	log_debug(debugLogger, "Enviando PCB a Nucleo...");
	int bytes = bytes_PCB(pcbActual);
	char* serialPCB = malloc(bytes);
	serializar_PCB(serialPCB, pcbActual);

	enviarLargoYSerial(nucleo, bytes, serialPCB);
	log_debug(debugLogger, "PCB Enviado!");
	free(serialPCB);
}

/**
 * Llamo a la funcion analizadorLinea del parser y logeo
 */
void parsear(char* const sentencia) {
	log_info(activeLogger, "Ejecutando la sentencia |%s|...", sentencia);
	pcbActual->PC++; //si desp el parser lo setea en otro lado mediante una primitiva, es tema suyo.
					//lo incremento antes asi no se desfasa.

	if(noEsEnd(sentencia)){
		analizadorLinea(sentencia, &funciones, &funcionesKernel);
		log_info(activeLogger, "PC actualizado a |%d|",pcbActual->PC);
		enviarHeader(nucleo,headerTermineInstruccion);
		log_debug(debugLogger,"Informé a nucleo del fin de una instrucción");
	}
	else
		finalizar_proceso(true);
}
/**
 * Recibo la sentencia previamente pedida.
 */
char* recibir_sentencia(int tamanio){
	log_debug(debugLogger, "Recibiendo sentencia de tamaño |%d|...", tamanio);
	char* sentencia = recv_waitall_ws(umc, tamanio);
	sentencia[tamanio-1]='\0';
	log_debug(debugLogger, "Recibida la sentencia: |%s|", sentencia);
	return sentencia;
}

/**
 * Loggeada en las funciones que llama
 */
void obtener_y_parsear() {
	recibir_quantum_sleep(); //TODO activar para la entrega.
	int tamanio;
	usleep(quantum_sleep); //TODO . le dejo marca porque lo uso para testear el kill -s y el orden de segmentfaulteo de los procesos.
	sentenciaPedida = string_new();
	pedirYRecibirSentencia(&tamanio);
	parsear(sentenciaPedida);
	sleep(quantum_sleep);
	free(sentenciaPedida);
}

/**
 * Lanza excepcion por stack overflow y termina el proceso.
 */
void lanzar_excepcion_overflow(){
	log_info(activeLogger,ANSI_COLOR_RED "Stack overflow! se intentó leer una dirección inválida." ANSI_COLOR_RESET);
	log_info(activeLogger,"Terminando la ejecución del programa actual...");

	overflow=true;
	finalizar_proceso(false);

	log_info(activeLogger,"Proceso terminado!");
}

// ***** Funciones de inicializacion y finalizacion ***** //
void cargarConfig() {
	t_config* configCPU;
	configCPU = config_create("cpu.cfg");
	config.puertoNucleo = config_get_int_value(configCPU, "PUERTO_NUCLEO");
	config.ipNucleo = config_get_string_value(configCPU, "IP_NUCLEO");
	config.puertoUMC = config_get_int_value(configCPU, "PUERTO_UMC");
	config.ipUMC = config_get_string_value(configCPU, "IP_UMC");
	config.DEBUG_RAISE_LOG_LEVEL = config_get_int_value(configCPU, "LOGGEAR_TODO");


	//	config.DEBUG_IGNORE_UMC = config_get_int_value(configCPU, "DEBUG_IGNORE_UMC");
	//	config.DEBUG_IGNORE_PROGRAMS = config_get_int_value(configCPU, "DEBUG_IGNORE_PROGRAMS");
	//	config.DEBUG_IGNORE_NUCLEO = config_get_int_value(configCPU, "DEBUG_IGNORE_NUCLEO");
	//	config.DEBUG_RUN_UNITARY_TESTS = config_get_int_value(configCPU, "DEBUG_RUN_UNITARY_TESTS");
	//	config.DEBUG_RUN_TESTS_WITH_UMC = config_get_int_value(configCPU, "DEBUG_RUN_TESTS_WITH_UMC");
	//	config.DEBUG_LOG_ON_TESTS = config_get_int_value(configCPU, "DEBUG_LOG_ON_TESTS");
		config.DEBUG_IGNORE_UMC = false;
		config.DEBUG_IGNORE_PROGRAMS = false;
		config.DEBUG_IGNORE_NUCLEO = false;
		config.DEBUG_RUN_UNITARY_TESTS = false;
		config.DEBUG_RUN_TESTS_WITH_UMC = false;
		config.DEBUG_LOG_ON_TESTS = false;
}

void inicializar() {
	cargarConfig();
	ejecutando = false;
	terminar = false;
	overflow = false;
	pcbActual = NULL; //lo dejo en NULL por chequeos en otro lado.
	crearLogs(string_from_format("cpu_%d", getpid()), "CPU", config.DEBUG_RAISE_LOG_LEVEL);
	log_info(activeLogger, "Soy CPU de process ID %d.", getpid());
	inicializar_primitivas();
}

void finalizar() {
	log_info(activeLogger,"Finalizando proceso cpu");

	close(nucleo);
	close(umc);

	log_info(activeLogger,"Proceso CPU de PID: |%d| finalizó correctamente.",getpid());
	destruirLogs();
	exit(EXIT_SUCCESS);
}

/*------------otras------------*/
/**
 * Para sigusr1 => terminar=1
 */
void handler(int sign) {
	if (sign == SIGUSR1) {
		log_info(activeLogger, ANSI_COLOR_RED "Recibi SIGUSR1!" ANSI_COLOR_RESET);
		terminar = true; //Setea el flag para que termine CPU al terminar de ejecutar la instruccion
		log_info(activeLogger, "Esperando a que termine la ejecucion del programa actual...");
	}
}

/**
 * Inicializa los tests si estan habilitados (1) por configuracion.
 */
void correrTests(){
	if(config.DEBUG_RUN_UNITARY_TESTS){
		if(!config.DEBUG_LOG_ON_TESTS){
			desactivarLogs();
		}
		testear(test_cpu);
		if(!config.DEBUG_LOG_ON_TESTS){
			reactivarLogs();
		}
	}
}

void correrTestsUMC(){
	if(config.DEBUG_RUN_TESTS_WITH_UMC && !config.DEBUG_IGNORE_UMC){
		if(!config.DEBUG_LOG_ON_TESTS){
			desactivarLogs();
		}
		testear(test_cpu_con_umc);
		if(!config.DEBUG_LOG_ON_TESTS){
			reactivarLogs();
		}
	}
}

int main() {
	signal(SIGUSR1,handler); //el progama sabe que cuando se recibe SIGUSR1,se ejecuta handler

	inicializar();

	//tests
	correrTests();

	//conectarse a umc
	establecerConexionConUMC();

	pedir_tamanio_paginas();

	//test con UMC
	correrTestsUMC();

	//ejemploPedidoLecturaUmc();

	//conectarse a nucleo
	establecerConexionConNucleo();

	//CPU se pone a esperar que nucleo le envie PCB
	esperar_programas();

	finalizar();
	return EXIT_SUCCESS;
}
