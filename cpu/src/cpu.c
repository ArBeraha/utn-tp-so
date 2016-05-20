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
	pcb->PC = pc;
}
void incrementarPC(t_PCB* pcb) {
	pcb->PC++;
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

t_pedido pedirMemoria() { //TODO hacer que esto pida memoria a umc
	t_pedido var;
	var.offset = 0;
	var.pagina = 0;
	var.size = sizeof(int);
	return var;
}

/*--------FUNCIONES----------*/

/*--------Primitivas----------*/

//cambiar el valor de retorno a t_puntero
t_puntero definir_variable(t_nombre_variable variable) {
	incrementarPC(pcbActual);

	t_pedido direccion = pedirMemoria();
	t_stack_item* head = stack_pop(stack);
	list_add(head->argumentos, (void*) &direccion);
	dictionary_put(head->identificadores, &variable, (void*) &direccion);
	stack_push(stack, head);

	informarInstruccionTerminada();
	instruccionTerminada("Definir_variable");
	return head->posicion;
}

t_puntero obtener_posicion_de(t_nombre_variable variable) {
	log_info(activeLogger, "Obtener posicion de |%c|.", variable);

	t_puntero pointer = -1;
	t_stack_item* head = stack_pop(stack);
	if (dictionary_has_key(head, (void*) &variable)) {
		t_pedido* direccion = dictionary_get(head, (void*) &variable);
		pointer = head->posicion;
		log_info(activeLogger,
				"Se encontro la variable |%c| en la posicion |%d|.", variable,
				pointer);
	} else {
		log_info(activeLogger, "No se encontro la variable |%c|., variable");
	}
	stack_push(stack, head);

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("obtener_posicion_de");
	return pointer;
}

void enviar_direccion_umc(t_puntero direccion) {
	t_stack_item* stackItem = stack_get(pcbActual->SP, direccion);
	t_pedido pedido = stackItem->valorRetorno;

	char* mensaje = string_new();
	serializar_variable(mensaje, &pedido);

	send_w(cliente_umc, mensaje, sizeof(t_pedido)); // envio el pedido [pag,offset,size]

	stack_item_destroy(stackItem);
	free(mensaje);
}

t_valor_variable dereferenciar(t_puntero direccion) {// TODO terminar - Pido a UMC el valor de la variable de direccion
	t_valor_variable valor;
	log_info(activeLogger, "Dereferenciar |%d| y su valor es:  ", direccion);

	send_w(cliente_umc, headerToMSG(HeaderPedirValorVariable), 1);

	enviar_direccion_umc(direccion);

	char* msgSize = recv_waitall_ws(cliente_umc, sizeof(int));
	int size = char4ToInt(msgSize);
	char* res = recv_waitall_ws(cliente_umc, size); //recibo el valor de UMC

	valor = charToInt(res);

	free(msgSize);
	free(res);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Dereferenciar");
	return valor;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor) {//TODO terminar
	log_info(activeLogger, "Asignando en |%d| el valor |%d|",
			direccion_variable, valor);
	send_w(cliente_umc, headerToMSG(HeaderAsignarValor), 1);

	enviar_direccion_umc(direccion_variable);

	send_w(cliente_umc, intToChar4(valor), sizeof(t_valor_variable));//envio el valor de la variable
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Asignar.");
}

t_valor_variable obtener_valor_compartida(t_nombre_compartida variable) { // Pido a Nucleo el valor de la variable
	log_info(activeLogger,
			"Obtener valor de variable compartida %s y es: |%d|.", variable,
			*variable); // no seria el valor real
	t_valor_variable valor;

	send_w(cliente_nucleo, headerToMSG(HeaderPedirValorVariableCompartida), 1);
	send_w(cliente_nucleo, variable, sizeof(t_nombre_compartida));

	char* msgSize = recv_waitall_ws(cliente_nucleo, sizeof(int));
	int size = char4ToInt(msgSize);
	char* res = recv_waitall_ws(cliente_nucleo, size);

	valor = charToInt(res);
	free(msgSize);
	free(res);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Obtener_valor_compartida");
	return valor;
}

t_valor_variable asignar_valor_compartida(t_nombre_compartida variable,
		t_valor_variable valor) {
	log_info(activeLogger,
			"Asignar el valor |%d| a la variable compartida |%s|.", valor,
			variable);

	send_w(cliente_nucleo, headerToMSG(HeaderAsignarValorVariableCompartida),
			1);		//envio el header

	send_w(cliente_nucleo, variable, sizeof(t_nombre_compartida));//envio el nombre de la variable

	char* valor_envio = intToChar4(valor);
	send_w(cliente_nucleo, valor_envio, sizeof(int));			//envio el valor

	//TODO esperar a que nucleo informe la asignacion, para no usar un valor antiguo.
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("asignar_valor_compartida");
	return valor;
}

//cambiar valor de retorno a t_puntero_instruccion
void irAlLaber(t_nombre_etiqueta etiqueta) {
	log_info(activeLogger, "Obtener puntero de |%s|.", etiqueta);
	int posicionEtiqueta = 0; // TODO acá va la de la etiqueta
	setearPC(pcbActual, posicionEtiqueta);
	informarInstruccionTerminada();
	instruccionTerminada("ir_al_laber");
}

//cambiar valor de retorno a t_puntero_instruccion
void llamar_sin_retorno(t_nombre_etiqueta nombreFuncion) {
	log_info(activeLogger, "Llamar a funcion |%s|.", nombreFuncion);
	int posicionFuncion = 0; // TODO acá va la de la funcion
	setearPC(pcbActual, posicionFuncion);
	informarInstruccionTerminada();
	instruccionTerminada("Llamar_sin_retorno");
}

//cambiar valor de retorno a t_puntero_instruccion
void retornar(t_valor_variable variable) {

	t_stack_item* stackItem; //TODO obtener stack!!
	t_puntero_instruccion retorno = stackItem->posicionRetorno;

	log_info(activeLogger,
			"Cambiar entorno actual usando el PC de |%d| a |%d|.",
			pcbActual->PC, retorno);

	setearPC(pcbActual, retorno);
	informarInstruccionTerminada();
	instruccionTerminada("Retornar");
}

void imprimir(t_valor_variable valor) {
	log_info(activeLogger, "Imprimir |%d|\n", valor);
	//header enviar valor de variable
	send_w(cliente_nucleo, intToChar(valor), sizeof(t_valor_variable));
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir");
}

void imprimir_texto(char* texto) {
	int size = strlen(texto) + 1; // El strlen no cuenta el \0. strlen("hola\0") = 4.
	log_debug(activeLogger, "Enviando a nucleo la cadena: |%s|...", texto);
	send_w(cliente_nucleo, headerToMSG(HeaderImprimirTextoNucleo), 1);
	send_w(cliente_nucleo, intToChar4(size), sizeof(int));
	send_w(cliente_nucleo, texto, size); //envio a nucleo la cadena a imprimir
	log_debug(activeLogger, "Se envio a nucleo la cadena: |%s|.", texto);
	// TODO ??? free(texto); //como no se que onda lo que hace la blbioteca, no se si tire segment fault al hacer free. Una vez q este todoo andando probar hacer free aca
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir texto");
}

//cambiar valor de retorno a int
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo) {
	log_info(activeLogger,
			"Informar a nucleo que el programa quiere usar |%s| durante |%d| unidades de tiempo",
			dispositivo, tiempo);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Entrada-Salida");
}

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

void signal(t_nombre_semaforo identificador_semaforo) {
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
	funciones.AnSISOP_llamarSinRetorno = &llamar_sin_retorno;
	funciones.AnSISOP_retornar = &retornar;
	funciones.AnSISOP_entradaSalida = &entrada_salida;
	funcionesKernel.AnSISOP_wait = &wait;
	funcionesKernel.AnSISOP_signal = &signal;
	log_info(activeLogger, "Primitivas Inicializadas");
}

void liberar_primitivas() {
	log_debug(bgLogger, "Liberando primitivas...");
	free(funciones.AnSISOP_definirVariable);
	free(funciones.AnSISOP_obtenerPosicionVariable);
	free(funciones.AnSISOP_dereferenciar);
	free(funciones.AnSISOP_asignar);
	free(funciones.AnSISOP_obtenerValorCompartida);
	free(funciones.AnSISOP_asignarValorCompartida);
	free(funciones.AnSISOP_irAlLabel);
	free(funciones.AnSISOP_imprimir);
	free(funciones.AnSISOP_imprimirTexto);
	free(funciones.AnSISOP_llamarSinRetorno);
	free(funciones.AnSISOP_retornar);
	free(funciones.AnSISOP_entradaSalida);
	free(funcionesKernel.AnSISOP_wait);
	free(funcionesKernel.AnSISOP_signal);
	log_debug(bgLogger, "Primitivas liberadas...");
}

void parsear(char* const sentencia) {
	log_info(activeLogger, "Ejecutando la setencia |%s|...", sentencia);
	analizadorLinea(sentencia, &funciones, &funcionesKernel);
}

/*--------Funciones----------*/

void pedir_tamanio_paginas() {
	send_w(cliente_umc, headerToMSG(HeaderTamanioPagina), 1); //le pido a umc el tamanio de las paginas
	char* tamanio = recv_nowait_ws(cliente_umc, sizeof(int)); //recibo el tamanio de las paginas
	tamanioPaginas = char4ToInt(tamanio);
	log_debug(activeLogger, "El tamaño de paginas es: |%d|", tamanioPaginas);
	free(tamanio);
}

void esperar_programas() {
	log_debug(bgLogger, "Esperando programas de nucleo %d.");
	char* header;
	while (1) {
		header = recv_waitall_ws(cliente_nucleo, 1);
		procesarHeader(header);
		free(header);
	}
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
	serializar_variable(solicitud, &pedido);

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
	char* header = recv_waitall_ws(cliente_nucleo, sizeof(char));
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
	char tamanioSentencia;
	read(cliente_umc, &tamanioSentencia, 1);

	char* sentencia = recv_waitall_ws(cliente_umc, tamanioSentencia);
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
	log_info(activeLogger, "Exito al conectar con NUCLEO!!");
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
	connect_w(cliente_umc, &direccionUmc); //conecto cpu a la direccion 'direccionUmc'
	log_info(activeLogger, "Exito al conectar con UMC!!");
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
void cargarConfig() { //fixme se rompe porque si!
//	t_config* configCPU;
//	configCPU = config_create("cpu.cfg");
//	config.puertoNucleo = config_get_int_value(configCPU, "PUERTO_NUCLEO");
//	config.ipNucleo = config_get_string_value(configCPU, "IP_NUCLEO");
//	config.puertoUMC = config_get_int_value(configCPU, "PUERTP_UMC");
//	config.ipUMC = config_get_string_value(configCPU, "IP_UMC");
	config.puertoNucleo=8080;
	config.ipNucleo="127.0.0.1";
	config.puertoUMC=8081;
	config.ipUMC="127.0.0.1";
}
void inicializar() {
	cargarConfig();
	pcbActual = malloc(sizeof(t_PCB));
	crearLogs(string_from_format("cpu_%d", getpid()), "CPU");
	log_info(activeLogger, "Soy CPU de process ID %d.", getpid());
	inicializar_primitivas();
}
void finalizar() {
	destruirLogs(); //fixme: no hay que usar las funciones que nos dan ellos?
	pcb_destroy(pcbActual);
	liberar_primitivas();
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
