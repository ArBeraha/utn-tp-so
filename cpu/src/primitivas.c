/*
 * primitivas.c
 *
 *  Created on: 31/5/2016
 *      Author: utnso
 */

#include "primitivas.h"
/*--------Primitivas----------*/

// Directiva 1.
//cambiar el valor de retorno a t_puntero.
t_puntero definir_variable(t_nombre_variable variable) {
	incrementarPC(pcbActual);

	t_pedido* direccion = stack_next_pedido(stack, tamanioPaginas);
	t_stack_item* head = stack_pop(stack);
	dictionary_put(head->identificadores, &variable, (void*) direccion);
	stack_push(stack, head);

	informarInstruccionTerminada();
	instruccionTerminada("Definir_variable");
	return head->posicion;
}

bool esVariableDeclarada(t_stack_item* item, t_nombre_variable* variable) {
	return dictionary_has_key(item->identificadores, variable);
}

bool esParametro(t_nombre_variable variable) {
	return (variable >= '0' && variable <= '9');
}

int tipoVaraible(t_nombre_variable variable, t_stack_item* head) {
	if (esVariableDeclarada(head, &variable)) {
		return DECLARADA;
	} else {
		if (esParametro(variable)) {
			return PARAMETRO;
		}
	}
	return NOEXISTE;
}

t_puntero obtener_posicion_de(t_nombre_variable variable) {
	log_info(activeLogger, "Obtener posicion de |%c|.", variable);
	t_puntero pointer;
	t_stack_item* head = stack_head(stack);
	switch (tipoVaraible(variable, head)) {
	case DECLARADA:
	case PARAMETRO:
		pointer = head->posicion;
		break;
	case NOEXISTE:
		pointer = -1;
		break;
	}

	if (pointer >= 0) {
		log_info(activeLogger,
				"Se encontro la variable |%c| en la posicion |%d|.", variable,
				pointer);
	} else {
		log_info(activeLogger, "No se encontro la variable |%c|.", variable);
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
t_valor_variable dereferenciar(t_puntero direccion) { // Pido a UMC el valor de la variable de direccion
	t_valor_variable valor;
	log_info(activeLogger, "Dereferenciar |%d|.", direccion);

	enviarHeader(cliente_umc,HeaderPedirValorVariable);
	enviar_direccion_umc(direccion);

	char* res = recv_waitall_ws(cliente_umc, sizeof(int)); //recibo el valor de UMC

	if(esExcepcion(res)){	//TODO como se efectivamente si es una excepcion? por ahi no lo es y la decodifica como tal

		free(res);
		lanzar_excepcion();

		}else{

			valor = charToInt(res);
			log_info(activeLogger, "|%d| dereferenciada! Su valor es |%d|.", direccion,
					valor);

			free(res);
			incrementarPC(pcbActual);
			informarInstruccionTerminada();
			instruccionTerminada("Dereferenciar");

	}
	return valor;
}

// Directiva 4
void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	log_info(activeLogger, "Asignando en |%d| el valor |%d|",
			direccion_variable, valor);

	enviarHeader(cliente_umc,HeaderAsignarValor);
	enviar_direccion_umc(direccion_variable);

	send_w(cliente_umc, intToChar4(valor), sizeof(t_valor_variable)); //envio el valor de la variable

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Asignar.");
}

// Directiva 5
t_valor_variable obtener_valor_compartida(t_nombre_compartida nombreVarCompartida) { // Pido a Nucleo el valor de la variable
	log_info(activeLogger, "Obtener valor de variable compartida |%s|.",nombreVarCompartida);
	t_valor_variable valor;
	int nameSize = strlen(nombreVarCompartida) + 1;

	enviarHeader(cliente_umc,HeaderPedirValorVariableCompartida);

	send_w(cliente_nucleo, intToChar4(nameSize), sizeof(int));
	send_w(cliente_nucleo, nombreVarCompartida, nameSize);

	char* value = recv_waitall_ws(cliente_nucleo, sizeof(int));
	valor = char4ToInt(value);

	log_info(activeLogger, "Valor obtenido: |%s| vale |%d|.",nombreVarCompartida, valor);
	free(value);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Obtener_valor_compartida");
	return valor;
}

//Directiva 6
t_valor_variable asignar_valor_compartida(t_nombre_compartida nombreVarCompartida, t_valor_variable valorVarCompartida) {
	log_info(activeLogger, //envio el nombre de la variable
			"Asignar el valor |%d| a la variable compartida |%s|.",
			valorVarCompartida, nombreVarCompartida);

	//envio el header, el tamaño del nombre y el nombre
	enviarHeader(cliente_umc, HeaderAsignarValorVariableCompartida);
	enviarLargoYString(cliente_nucleo,nombreVarCompartida);

	//envio el valor
	char* valor = intToChar4(valorVarCompartida);
	send_w(cliente_nucleo, valor, sizeof(int));

	//Espero a que nucleo informe la asignacion, para no usar un valor antiguo.
	char* respuesta = recv_nowait_ws(cliente_nucleo, 1);
	if (!charToInt(respuesta) == HeaderAsigneValorVariableCompartida) {
		log_error(activeLogger,
				"Se esperaba que nucleo asigne el valor |%d| a la variable compartida |%s| y no sucedio",
				valorVarCompartida, nombreVarCompartida);
		log_info(activeLogger, "Se continua la ejecucion de todas formas");
	} else {
		log_info(activeLogger,
				"Asignado el valor |%d| a la variable compartida |%s|.",
				valorVarCompartida, nombreVarCompartida);
	}

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("asignar_valor_compartida");
	return valorVarCompartida;
}

bool existeLabel(t_nombre_etiqueta etiqueta) {
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
void llamar_con_retorno(t_nombre_etiqueta nombreFuncion,t_puntero dondeRetornar) {
	log_info(activeLogger, "Llamar a funcion |%s|.", nombreFuncion);
	int posicionFuncion = 0; // TODO acá va la de la funcion

	t_stack_item* newHead = stack_item_create();
	//newHead->argumentos; //fixme: ¿Como lleno esto? stack_next_pedido parece ser re util
	//aca, pero no se como saber cuantos argumentos tengo :(
	//parsear la linea a mano no me parece una solucion, pese a que funcionaria...
	//newHead->identificadores no tiene nada por ahora. Se va llenando en otras primitivas, a medida que se declaren variables locales.
	newHead->posicionRetorno = dondeRetornar;
	newHead->posicion = stack_size(stack); // Si el stack tiene pos 0, size=1, si tiene 0 y 1, size=2,... Da la posicion del lugar nuevo.
	newHead->valorRetorno = *stack_max_pedido(stack, tamanioPaginas);
	stack_push(stack, newHead);

	setearPC(pcbActual, posicionFuncion);
	informarInstruccionTerminada();
	instruccionTerminada("llamar_con_retorno");
}

//Directiva 9
t_puntero_instruccion retornar(t_valor_variable variable) {
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
	return retorno;
}

int digitosDe(t_valor_variable valor) {
	return snprintf(0, 0, "%d", valor);
}

// Directiva 10
// fixme: tipo incompatible con el del enunciado! no borrar el return comentado!
void imprimir(t_valor_variable valor) { //fixme, no era distinto esto?
	log_info(activeLogger, "Imprimir |%d|", valor);

	enviarHeader(cliente_nucleo,HeaderImprimirVariableNucleo);
	send_w(cliente_nucleo, intToChar4(valor), sizeof(t_valor_variable));
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir");
	//return digitosDe(valor);
}

//Directiva 11
// fixme: tipo incompatible con el del enunciado! no borrar el return comentado!
void imprimir_texto(char* texto) {

	log_debug(activeLogger, "Enviando a nucleo la cadena: |%s|...", texto);

	enviarHeader(cliente_nucleo, HeaderImprimirTextoNucleo);

	enviarLargoYString(cliente_nucleo, texto);

	log_debug(activeLogger, "Se envio a nucleo la cadena: |%s|.", texto);

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir texto");
	//return strlen(texto); //Size tiene el \0, que no se imprime.
}

// Directiva 12
//cambiar valor de retorno a int
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo) {
	log_info(activeLogger,"Informar a nucleo que el programa quiere usar |%s| durante |%d| unidades de tiempo",
			dispositivo, tiempo);

	enviarHeader(cliente_nucleo,HeaderEntradaSalida);

	enviarLargoYString(cliente_nucleo,dispositivo);				//envio la cadena

	char* time = intToChar(tiempo);							//envio el tiempo
	send_w(cliente_nucleo,time,strlen(time));
	free(time);

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Entrada-Salida");
}


// Directiva 13
void wait(t_nombre_semaforo identificador_semaforo) {
	log_info(activeLogger, "Comunicar nucleo de hacer wait con semaforo: |%s|",
			identificador_semaforo);

	enviarHeader(cliente_nucleo,HeaderWait);

	enviarLargoYString(cliente_nucleo,identificador_semaforo);


	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("wait");
}

// Directiva 14
void signal_con_semaforo(t_nombre_semaforo identificador_semaforo) {
	log_info(activeLogger,"Comunicar nucleo de hacer signal con semaforo: |%s|",identificador_semaforo);

	enviarHeader(cliente_nucleo,HeaderSignal);

	enviarLargoYString(cliente_nucleo,identificador_semaforo);

	incrementarPC(pcbActual);
	informarInstruccionTerminada();
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
	log_info(activeLogger, "Primitivas Inicializadas.");
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
