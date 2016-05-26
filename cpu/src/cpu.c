/*
 * cpu.c
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "cpu.h"

/*------------Declaracion de funciones--------------*/
void procesarHeader(char*);
void esperar_programas();
void pedir_sentencia();
void parsear();
void obtenerPCB();
void esperar_sentencia();
void obtener_y_parsear();

/*----- Operaciones sobre el PC y avisos por quantum -----*/
void informarInstruccionTerminada() {
	// Le aviso a nucleo que termino una instruccion, para que calcule cuanto quantum le queda al proceso ansisop.
	send_w(cliente_nucleo, headerToMSG(headerTermineInstruccion), 1);
	// Acá nucleo tiene que mandarme el header que corresponda, segun si tengo que seguir ejecutando instrucciones o tengo que desalojar.
}
void setearPC(t_PCB* pcb, int pc) {
	log_info(activeLogger, "Actualizando PC de |%d| a |%d|.", pcb->PC, pc);
	pcb->PC = pc;
}
void incrementarPC(t_PCB* pcb) {
	setearPC(pcb,(pcb->PC)+1);
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

t_pedido maximo(t_pedido pedido1, t_pedido pedido2){ //Determina que pedido está mas lejos respecto del inicio!
	if(pedido1.pagina>pedido2.pagina){
		return pedido1;
	}
	if(pedido1.pagina==pedido2.pagina){
		return (pedido1.offset>pedido2.offset?pedido1:pedido2);
	}
	return pedido2;
}

/*--------FUNCIONES----------*/

/*--------Primitivas----------*/

// Directiva 1.
//cambiar el valor de retorno a t_puntero.
t_puntero definir_variable(t_nombre_variable variable) {
	incrementarPC(pcbActual);

	t_pedido* direccion = stack_next_pedido(stack,tamanioPaginas);
	t_stack_item* head = stack_pop(stack);
	dictionary_put(head->identificadores, &variable, (void*) direccion);
	stack_push(stack, head);

	informarInstruccionTerminada();
	instruccionTerminada("Definir_variable");
	return head->posicion;
}

bool esVariableDeclarada(t_stack_item* item, t_nombre_variable* variable){
	return dictionary_has_key(item->identificadores, variable);
}

bool esParametro(t_nombre_variable variable){
	return (variable>='0' && variable<='9');
}

int tipoVaraible(t_nombre_variable variable,t_stack_item* head){
	if(esVariableDeclarada(head, &variable)){
		return DECLARADA;
	}else{
		if(esParametro(variable)){
			return PARAMETRO;
		}
	}
	return NOEXISTE;
}

t_puntero obtener_posicion_de(t_nombre_variable variable) {			//dejo comentada la otra solucion por si las dudas
	log_info(activeLogger, "Obtener posicion de |%c|.", variable);
	t_puntero pointer;
	t_stack_item* head = stack_head(stack);
	switch(tipoVaraible(variable,head)){
	case DECLARADA:
		pointer = head->posicion;
		break;
	case PARAMETRO:
		pointer = head->posicion;
		break;
	case NOEXISTE:
		pointer = -1;
		break;
	}

	if(pointer < 0 ){
		log_info(activeLogger,
				"Se encontro la variable |%c| en la posicion |%d|.", variable,
				pointer);
	} else{
		log_info(activeLogger, "No se encontro la variable |%c|., variable");
	}

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("obtener_posicion_de");
	return pointer;
}


void enviar_direccion_umc(t_puntero direccion) {
	t_stack_item* stackItem = stack_get(pcbActual->SP, direccion);
	t_pedido pedido = stackItem->valorRetorno;

	char* mensaje = string_new();
	serializar_pedido(mensaje, &pedido);

	send_w(cliente_umc, mensaje, sizeof(t_pedido)); // envio el pedido [pag,offset,size]

	free(mensaje);
}

// directiva 3
t_valor_variable dereferenciar(t_puntero direccion) {// Pido a UMC el valor de la variable de direccion
	t_valor_variable valor;
	log_info(activeLogger, "Dereferenciar |%d|.", direccion);

	send_w(cliente_umc, headerToMSG(HeaderPedirValorVariable), 1);
	enviar_direccion_umc(direccion);
	char* res = recv_waitall_ws(cliente_umc, sizeof(int)); //recibo el valor de UMC
	valor = charToInt(res);
	log_info(activeLogger, "|%d| dereferenciada! Su valor es |%d|.", direccion, valor);

	free(res);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Dereferenciar");
	return valor;
}

// Directiva 4
void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	log_info(activeLogger, "Asignando en |%d| el valor |%d|",
			direccion_variable, valor);

	send_w(cliente_umc, headerToMSG(HeaderAsignarValor), 1);
	enviar_direccion_umc(direccion_variable);
	send_w(cliente_umc, intToChar4(valor), sizeof(t_valor_variable));//envio el valor de la variable

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Asignar.");
}

// Directiva 5
t_valor_variable obtener_valor_compartida(t_nombre_compartida nombreVarCompartida) { // Pido a Nucleo el valor de la variable
	log_info(activeLogger,
			"Obtener valor de variable compartida |%s|.", nombreVarCompartida);
	t_valor_variable valor;
	int nameSize = strlen(nombreVarCompartida)+1;

	send_w(cliente_nucleo, headerToMSG(HeaderPedirValorVariableCompartida), 1);
	send_w(cliente_nucleo, intToChar4(nameSize), sizeof(int));
	send_w(cliente_nucleo, nombreVarCompartida, nameSize);

	char* value = recv_waitall_ws(cliente_nucleo, sizeof(int));
	valor = char4ToInt(value);

	log_info(activeLogger,
				"Valor obtenido: |%s| vale |%d|.", nombreVarCompartida, valor);
	free(value);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Obtener_valor_compartida");
	return valor;
}

//Directiva 6
t_valor_variable asignar_valor_compartida(t_nombre_compartida nombreVarCompartida,
		t_valor_variable valorVarCompartida) {
	log_info(activeLogger,//envio el nombre de la variable
			"Asignar el valor |%d| a la variable compartida |%s|.", valorVarCompartida,
			nombreVarCompartida);
	int nameSize = strlen(nombreVarCompartida)+1;

	//envio el header, el tamaño del nombre y el nombre
	send_w(cliente_nucleo, headerToMSG(HeaderAsignarValorVariableCompartida), 1);
	send_w(cliente_nucleo, intToChar4(nameSize), sizeof(int));
	send_w(cliente_nucleo, nombreVarCompartida, nameSize);

	//envio el valor
	char* valor = intToChar4(valorVarCompartida);
	send_w(cliente_nucleo, valor, sizeof(int));

	//Espero a que nucleo informe la asignacion, para no usar un valor antiguo.
	char* respuesta = recv_nowait_ws(cliente_nucleo, 1);
	if(!charToInt(respuesta)==HeaderAsigneValorVariableCompartida){
		log_error(activeLogger,
				"Se esperaba que nucleo asigne el valor |%d| a la variable compartida |%s| y no sucedio",
				valorVarCompartida, nombreVarCompartida);
		log_info(activeLogger, "Se continua la ejecucion de todas formas");
	}else{
		log_info(activeLogger,
				"Asignado el valor |%d| a la variable compartida |%s|.", valorVarCompartida,
				nombreVarCompartida);
	}

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("asignar_valor_compartida");
	return valorVarCompartida;
}


bool existeLabel(t_nombre_etiqueta etiqueta){
	return dictionary_has_key(pcbActual->indice_etiquetas, etiqueta);
}

//Directiva 7
// fixme: tipo incompatible con el del enunciado! no borrar el return comentado!
void irAlLaber(t_nombre_etiqueta etiqueta) {
	log_info(activeLogger, "Ir a la etiqueta |%s|.", etiqueta);
	t_puntero_instruccion posicionPrimeraInstrUtil = -1;
	if (existeLabel(etiqueta)) {
		// Casteo el puntero a void como puntero a int y despunterizo eso: void*->t_puntero_instruccion*, y t_puntero_instruccion*->t_puntero_instruccion.
		posicionPrimeraInstrUtil = *(t_puntero_instruccion*) dictionary_get(
				pcbActual->indice_etiquetas, etiqueta);
		log_info(activeLogger, "La etiqueta |%s| existe y tiene posición |%d|.",
				etiqueta, posicionPrimeraInstrUtil);
	} else {
		log_info(activeLogger,
				"La etiqueta |%s| no existe, por lo que la posición a retornar será -1.",
				etiqueta);
	}
	setearPC(pcbActual, posicionPrimeraInstrUtil);
	informarInstruccionTerminada();
	instruccionTerminada("ir_al_laber");
	//return posicionPrimeraInstrUtil;
}

//Directiva 8
//Cambio respecto de la version inicial del enunciado! esta version es acorde a la nueva.
void llamar_con_retorno(t_nombre_etiqueta nombreFuncion, t_puntero dondeRetornar) {
	log_info(activeLogger, "Llamar a funcion |%s|.", nombreFuncion);
	int posicionFuncion = 0; // TODO acá va la de la funcion

	t_stack_item* newHead;
	newHead->posicionRetorno=dondeRetornar;


	setearPC(pcbActual, posicionFuncion);
	informarInstruccionTerminada();
	instruccionTerminada("llamar_con_retorno");
}

//Directiva 9
//TODO cambiar valor de retorno a t_puntero_instruccion
void retornar(t_valor_variable variable) {
	t_stack_item* head = stack_pop(stack);
	t_puntero_instruccion retorno = head->posicionRetorno;
	log_info(activeLogger,
			"Se va a cambiar el PC de |%d| a |%d| debido a la directiva 'retornar'.",
			pcbActual->PC, retorno);

	// Libero ese nivel del stack, porque termino de ejecutarse la funcion que lo creo y ya no es necesario
	stack_item_destroy(head);
	setearPC(pcbActual, retorno);
	informarInstruccionTerminada();
	instruccionTerminada("Retornar");
}

int digitosDe(t_valor_variable valor){
	bool esPositivo = valor > 0;
	int digitos = snprintf(0, 0, "%d", valor);
	return esPositivo ? digitos : digitos + 1;
}

// Directiva 10
// fixme: tipo incompatible con el del enunciado! no borrar el return comentado!
void imprimir(t_valor_variable valor) { //fixme, no era distinto esto?
	log_info(activeLogger, "Imprimir |%d|", valor);
	send_w(cliente_nucleo, headerToMSG(HeaderImprimirVariableNucleo), 1);
	send_w(cliente_nucleo, intToChar4(valor), sizeof(t_valor_variable));
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir");
	//return digitosDe(valor);
}

//Directiva 11
// fixme: tipo incompatible con el del enunciado! no borrar el return comentado!
void imprimir_texto(char* texto) {
	int size = strlen(texto) + 1; // El strlen no cuenta el \0. strlen("hola\0") = 4.
	log_debug(activeLogger, "Enviando a nucleo la cadena: |%s|...", texto);
	send_w(cliente_nucleo, headerToMSG(HeaderImprimirTextoNucleo), 1);
	char* msgSize = NULL;
	serializar_int(msgSize, &size);
	send_w(cliente_nucleo, msgSize, sizeof(int)); //intToChar4 produce un memory leak si no se usa free
	send_w(cliente_nucleo, texto, size); //envio a nucleo la cadena a imprimir
	log_debug(activeLogger, "Se envio a nucleo la cadena: |%s|.", texto);
	// TODO ??? free(texto); //como no se que onda lo que hace la blbioteca, no se si tire segment fault al hacer free. Una vez q este todoo andando probar hacer free aca
	free(msgSize);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir texto");
	//return strlen(texto); //Size tiene el \0, que no se imprime.
}

// Directiva 12
//cambiar valor de retorno a int
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo) {
	log_info(activeLogger,
			"Informar a nucleo que el programa quiere usar |%s| durante |%d| unidades de tiempo",
			dispositivo, tiempo);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Entrada-Salida");
}

// Directiva 13
void wait(t_nombre_semaforo identificador_semaforo) {
	log_info(activeLogger, "Comunicar nucleo de hacer wait con semaforo: |%s|",
			identificador_semaforo);
	send_w(cliente_nucleo, headerToMSG(HeaderWait), 1);
	send_w(cliente_nucleo, identificador_semaforo,
			sizeof(identificador_semaforo));
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("wait");
}

// Directiva 14
void signal_con_semaforo(t_nombre_semaforo identificador_semaforo) {
	log_info(activeLogger,
			"Comunicar nucleo de hacer signal con semaforo: |%s|",
			identificador_semaforo);
	send_w(cliente_nucleo, headerToMSG(HeaderSignal), 1);
	send_w(cliente_nucleo, identificador_semaforo,
			sizeof(identificador_semaforo));
	incrementarPC(pcbActual);
	instruccionTerminada("Signal");
}

/* ------ Funciones para usar con el parser ----- */
void inicializar_primitivas() {
	log_info(bgLogger, "Inicianlizando primitivas...");
	funciones.AnSISOP_definirVariable = &definir_variable;
	funciones.AnSISOP_obtenerPosicionVariable = &obtener_posicion_de;
	funciones.AnSISOP_dereferenciar = &dereferenciar;
	funciones.AnSISOP_asignar = &asignar;
	funciones.AnSISOP_obtenerValorCompartida = &obtener_valor_compartida;
	funciones.AnSISOP_asignarValorCompartida = &asignar_valor_compartida;
	funciones.AnSISOP_irAlLabel = &irAlLaber;
	funciones.AnSISOP_imprimir = &imprimir;
	funciones.AnSISOP_imprimirTexto = &imprimir_texto;
	funciones.AnSISOP_llamarConRetorno = &llamar_con_retorno;
	funciones.AnSISOP_retornar = &retornar;
	funciones.AnSISOP_entradaSalida = &entrada_salida;
	funcionesKernel.AnSISOP_wait = &wait;
	funcionesKernel.AnSISOP_signal = &signal_con_semaforo;
	log_info(activeLogger, "Primitivas Inicializadas");
}

void liberar_primitivas() { //FIXME: comento estas lineas porque tiran segment fault.
	log_debug(bgLogger, "Liberando primitivas...");
//	free(funciones.AnSISOP_definirVariable);
//	free(funciones.AnSISOP_obtenerPosicionVariable);
//	free(funciones.AnSISOP_dereferenciar);
//	free(funciones.AnSISOP_asignar);
//	free(funciones.AnSISOP_obtenerValorCompartida);
//	free(funciones.AnSISOP_asignarValorCompartida);
//	free(funciones.AnSISOP_irAlLabel);
//	free(funciones.AnSISOP_imprimir);
//	free(funciones.AnSISOP_imprimirTexto);
//	free(funciones.AnSISOP_llamarSinRetorno);
//	free(funciones.AnSISOP_retornar);
//	free(funciones.AnSISOP_entradaSalida);
//	free(funcionesKernel.AnSISOP_wait);
//	free(funcionesKernel.AnSISOP_signal);
	log_debug(bgLogger, "Primitivas liberadas...");
}

void parsear(char* const sentencia) {
	log_info(activeLogger, "Ejecutando la setencia |%s|...", sentencia);
	analizadorLinea(sentencia, &funciones, &funcionesKernel);
}

/*--------Funciones----------*/

void pedir_tamanio_paginas() {
	if(!DEBUG_IGNORE_UMC){
		send_w(cliente_umc, headerToMSG(HeaderTamanioPagina), 1); //le pido a umc el tamanio de las paginas
		char* tamanio = recv_nowait_ws(cliente_umc, sizeof(int)); //recibo el tamanio de las paginas
		tamanioPaginas = char4ToInt(tamanio);
		log_debug(activeLogger, "El tamaño de paginas es: |%d|", tamanioPaginas);
		free(tamanio);
	}else{
		tamanioPaginas = -1;
		log_debug(activeLogger, "UMC DEBUG ACTIVADO! tamanioPaginas va a valer -1.");
	}
}

void esperar_programas() {
	log_debug(bgLogger, "Esperando programas de nucleo.");
	char* header;
	if(DEBUG_NO_PROGRAMS){
		log_debug(activeLogger, "DEBUG NO PROGRAMS activado! Ignorando programas...");
	}
	while (!DEBUG_NO_PROGRAMS) {
		header = recv_waitall_ws(cliente_nucleo, 1);
		procesarHeader(header);
		free(header);
	}
	log_debug(bgLogger, "Ya no se esperan programas de nucleo.");
}

void procesarHeader(char *header) {
	// Segun el protocolo procesamos el header del mensaje recibido
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
		obtenerPCB(); //inicio el proceso de aumentar el PC, pedir UMC sentencia...
		break;

	case HeaderSentencia:
		obtener_y_parsear();
		break;

	case HeaderDesalojarProceso:
		desalojarProceso();
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

int longitud_sentencia(t_sentencia* sentencia) {
	return sentencia->offset_fin - sentencia->offset_inicio;
}

int obtener_offset_relativo(t_sentencia* fuente, t_sentencia* destino) {
	int offsetInicio = fuente->offset_inicio;
	int numeroPagina = (int) (offsetInicio / tamanioPaginas); //obtengo el numero de pagina
	int offsetRelativo = (int) offsetInicio % tamanioPaginas;//obtengo el offset relativo

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
	serializar_pedido(solicitud, &pedido);

	send_w(cliente_umc, solicitud, sizeof(t_pedido));
	free(solicitud);

}

void pedir_sentencia() {	//pedir al UMC la proxima sentencia a ejecutar
	int entrada = pcbActual->PC; //obtengo la entrada de la instruccion a ejecutar

	t_sentencia* sentenciaActual = list_get(pcbActual->indice_codigo, entrada);	//obtengo el offset de la sentencia
	t_sentencia* sentenciaAux = malloc(sizeof(t_sentencia));

	int pagina = obtener_offset_relativo(sentenciaActual, sentenciaAux);//obtengo el offset relativo

	send_w(cliente_umc, headerToMSG(HeaderSolicitudSentencia), 1); //envio el header

	int i = 0;
	int longitud_restante = longitud_sentencia(sentenciaAux);//longitud total de la sentencia
	int cantidad_pags = cantidad_paginas_ocupa(sentenciaAux);

	log_debug(bgLogger, "La instruccion ocupa |%d| paginas", cantidad_pags);

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

void enviarPCB(){
	char* pcb = string_new();
	serializar_PCB(pcb,pcbActual);

	send_w(cliente_nucleo,pcb,sizeof(t_PCB));
	free(pcb);
}

void obtener_y_parsear() {
	char* tamanioSentencia = recv_waitall_ws(cliente_umc, sizeof(int));
	int tamanio = char4ToInt(tamanioSentencia);
	char* sentencia = recv_waitall_ws(cliente_umc, tamanio);
	parsear(sentencia);
	free(sentencia);
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
	log_info(activeLogger, "Conectado a nucleo!");
}

void hacer_handshake_nucleo() {
	char* hand = string_from_format("%c%c", HeaderHandshake, SOYCPU);
	send_w(cliente_nucleo, hand, 2);

	if (getHandshake(cliente_nucleo) != SOYNUCLEO) {
		perror("Se esperaba que CPU se conecte con el nucleo.");
	} else {
		log_info(bgLogger, "Exito al hacer handshake con nucleo.");
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
	if (!DEBUG_IGNORE_UMC) {
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
}
void inicializar() {
	cargarConfig();
	pcbActual = malloc(sizeof(t_PCB));
	crearLogs(string_from_format("cpu_%d", getpid()), "CPU");
	log_info(activeLogger, "Soy CPU de process ID %d.", getpid());
	inicializar_primitivas();
}
void finalizar() {
	if(!DEBUG_NO_PROGRAMS){
		log_debug(activeLogger,"Destruyendo pcb...");
		pcb_destroy(pcbActual);
		log_debug(activeLogger,"PCB Destruido...");
	}
	liberar_primitivas();
	destruirLogs();

	close(cliente_nucleo);
	close(cliente_umc);
	exit(EXIT_SUCCESS);
}

/*------------otras------------*/
void handler(int sign){
	if(sign== SIGUSR1){
		log_info(activeLogger, "Recibi SIGUSR1! Adios a todos!");
		finalizar();
		//desconectar de UMC y Nucleo, pero primero deberia terminar de ejecutar el proceso actual
	}
	else{
		log_info(activeLogger, "Recibi la señal numero |%d|, que no es SIGUSER1.", sign);
	}
}

int main() {
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
