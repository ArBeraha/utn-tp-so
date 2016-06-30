/*
 * primitivas.c
 *
 *  Created on: 31/5/2016
 *      Author: utnso
 */

/**
 * Cobertura:  ^ = Protocolo OK
 *  ^ ^ = Funciona OK
 */


#include "primitivas.h"
/*--------Primitivas----------*/

/**
 * Directiva 1
 */
t_puntero definir_variable(t_nombre_variable variable) {

	t_pedido* direccion = stack_next_pedido(stack, tamanioPaginas);
	t_stack_item* head = stack_pop(stack);
	char* cadena = charToString((char)variable);

	if(esParametro(variable))
	{
		list_add(head->argumentos,(void*)direccion);
	}else{
		dictionary_put(head->identificadores, cadena, (void*) direccion); //agrego el caracter a una cadena
	}

	stack_push(stack, head);
//	head->posicion = stack_size(stack) - 1; // si size es 1 -> pos = 0. Se calcula con el elemento ya agregado!

	free(cadena);

	loggearFinDePrimitiva("Definir_variable");
	return head->posicion;
}


/**
 * Directiva 2
 */
t_puntero obtener_posicion_de(t_nombre_variable variable) {
	log_info(activeLogger, "Obtener posicion de |%c|.", variable);
	t_puntero posicionAbsoluta = 0; //no sacar esta inicializacion por el if de abajo
	t_pedido* posicionRelativa;
	char* cadena = charToString((char)variable);
	t_stack_item* head = stack_head(stack);
	switch (tipoVaraible(variable, head)) {
	case DECLARADA:
		posicionRelativa = (t_pedido*)dictionary_get(head->identificadores,cadena);
		break;
	case PARAMETRO:
		posicionRelativa = (t_pedido*)list_get(head->argumentos,nombreToInt(variable)); //fixme: si rompe, sumarle 1 a la posicion.
		break;
	case NOEXISTE:
		posicionAbsoluta = -1;
		break;
	}

	if (posicionAbsoluta >= 0) {
		posicionAbsoluta = posicionRelativa->pagina*tamanioPaginas + posicionRelativa->offset;
		log_info(activeLogger,
				"Se encontro la variable |%c| en la posicion: absoluta |%d|.", variable,
				posicionAbsoluta);
	} else {
		log_info(activeLogger, "No se encontro la variable |%c|.", variable);
	}


	free(cadena);
	loggearFinDePrimitiva("obtener_posicion_de");
	return posicionAbsoluta;
}


/**
 * Directiva 3
 */
t_valor_variable dereferenciar(t_puntero direccion) { // Pido a UMC el valor de la variable de direccion
	t_valor_variable valor;
	log_info(activeLogger, "Obtener valor de la posicion absoluta |%d|.", direccion);

	enviarHeader(umc, HeaderPedirValorVariable);
	enviar_direccion_umc(direccion); // esto chequea q no haya overflow

	if(!hayOverflow()){
		char* valorRecibido = recv_waitall_ws(umc, sizeof(int)); //recibo el valor de UMC
		valor = char4ToInt(valorRecibido);
		log_info(activeLogger, "La variable de la dirección fue |%d| dereferenciada! Su valor es |%d|.",
					direccion, valor);

		free(valorRecibido);

		loggearFinDePrimitiva("Dereferenciar");
		return valor;
	}else{
		lanzar_excepcion_overflow(overflow);
	}
	return 0;
}


/**
 * Directiva 4
 */
void asignar(t_puntero direccion_variable, t_valor_variable valor) {
	log_info(activeLogger, "Asignando en la posicion |%d| el valor |%d|", direccion_variable, valor);

	enviarHeader(umc,HeaderAsignarValor);
	enviar_direccion_umc(direccion_variable); // esto chequea q no haya overflow
	if(!hayOverflow()){
		char* valorSerializado = intToChar4(valor);
		send_w(umc, valorSerializado, sizeof(t_valor_variable)); //envio el valor de la variable


		free(valorSerializado);
		loggearFinDePrimitiva("Asignar.");
	}else{
		lanzar_excepcion_overflow(overflow);
	}
}


/**
 * Directiva 5 ^
 */
t_valor_variable obtener_valor_compartida(t_nombre_compartida nombreVarCompartida) { // Pido a Nucleo el valor de la variable
	log_info(activeLogger, "Obtener valor de variable compartida |%s|.",nombreVarCompartida);
	t_valor_variable valorVarCompartida;
	int nameSize = strlen(nombreVarCompartida) + 1;

	enviarHeader(nucleo,HeaderPedirValorVariableCompartida);

	char* sizeSerializado = intToChar4(nameSize);
	send_w(nucleo, sizeSerializado, sizeof(int));
	send_w(nucleo, nombreVarCompartida, nameSize);

	char* value = recv_waitall_ws(nucleo, sizeof(int));
	valorVarCompartida = char4ToInt(value);

	log_info(activeLogger, "Valor obtenido: |%s| vale |%d|.",nombreVarCompartida, valorVarCompartida);
	free(value);
	free(sizeSerializado);

	loggearFinDePrimitiva("Obtener_valor_compartida");
	return valorVarCompartida;
}


/**
 * Directiva 6 ^
 */
t_valor_variable asignar_valor_compartida(t_nombre_compartida nombreVarCompartida, t_valor_variable valorVarCompartida) {
	log_info(activeLogger, //envio el nombre de la variable
			"Asignar el valor |%d| a la variable compartida |%s|.",
			valorVarCompartida, nombreVarCompartida);

	//envio el header, el tamaño del nombre y el nombre
	enviarHeader(nucleo, HeaderAsignarValorVariableCompartida);
	enviarLargoYString(nucleo,nombreVarCompartida);

	//envio el valor
	char* valor = intToChar4(valorVarCompartida);
	send_w(nucleo, valor, sizeof(int));

	log_info(activeLogger,
				"Asignado el valor |%d| a la variable compartida |%s|.",
				valorVarCompartida, nombreVarCompartida);


	loggearFinDePrimitiva("asignar_valor_compartida");
	return valorVarCompartida;
}


/**
 * Directiva 7
 */
void irAlLabel(t_nombre_etiqueta etiqueta) {

	log_info(activeLogger, "Ir a la etiqueta |%s|.", etiqueta);
	t_puntero_instruccion posicionPrimeraInstrUtil = -1;
	if (existeLabel(etiqueta)) {

		posicionPrimeraInstrUtil = obtenerPosicionLabel(etiqueta);

		log_info(activeLogger, "La etiqueta |%s| existe y tiene posición |%d|.",
				etiqueta, posicionPrimeraInstrUtil);

	} else {

		log_info(activeLogger,
				"La etiqueta |%s| no existe, por lo que la posición a retornar será -1.",
				etiqueta);
	}
	setearPC(pcbActual, posicionPrimeraInstrUtil);
	loggearFinDePrimitiva("ir_al_label");
}

/**
 * Directiva 8
 */
void llamar_con_retorno(t_nombre_etiqueta nombreFuncion,t_puntero dondeRetornar) {
	log_info(activeLogger, "Llamar a funcion |%s|.", nombreFuncion);
	t_puntero_instruccion posicionFuncion =  obtenerPosicionLabel(nombreFuncion);

	t_stack_item* newHead = stack_item_create();
	//newHead->argumentos El parser llama a definir variable y se ocupa de esto
	//newHead->identificadores no tiene nada por ahora. Se va llenando en otras primitivas, a medida que se declaren variables locales.
	newHead->posicionRetorno = pcbActual->PC; //dondeRetornar;
	newHead->posicion = stack_size(stack); // Si el stack tiene pos 0, size=1, si tiene 0 y 1, size=2,... Da la posicion del lugar nuevo.
	//newHead->valorDeRetorno es el parser quien en retornar le pasa en que variable guardar el resultado.
	stack_push(stack, newHead);

	setearPC(pcbActual, posicionFuncion);
	loggearFinDePrimitiva("llamar_con_retorno");
}

/**
 * Directiva 9
 */
void retornar(t_valor_variable variable) {
	t_stack_item* head = stack_pop(stack);
	t_puntero_instruccion retorno = head->posicionRetorno;
	log_info(activeLogger,
			"Se va a cambiar el PC de |%d| a |%d| debido a la directiva 'retornar'.",
			pcbActual->PC, retorno);

	// Libero ese nivel del stack, porque termino de ejecutarse la funcion que lo creo y ya no es necesario
	stack_item_destroy(head);
	setearPC(pcbActual, retorno);
	loggearFinDePrimitiva("Retornar");
}


/**
 * Directiva 10
 */
void imprimir_variable(t_valor_variable valor) { //la nueva version del enunciado solo pasa el valor, no el nombre
	log_info(activeLogger, "Imprimir |%d|", valor);

	enviarHeader(nucleo,HeaderImprimirVariableNucleo);
	char* valorSerializado = intToChar4(valor);
	send_w(nucleo, valorSerializado, sizeof(t_valor_variable));

	free(valorSerializado); //hacer intToChar4 en el send produce memory leaks, porque al terminar al funcion queda memoria desreferenciada que nunca se libera.
	loggearFinDePrimitiva("Imprimir");
}


/**
 * Directiva 11
 */
void imprimir_texto(char* texto) {

	log_debug(debugLogger, "Enviando a nucleo la cadena: |%s|...", texto);

	enviarHeader(nucleo, HeaderImprimirTextoNucleo);

	enviarLargoYString(nucleo, texto);

	log_debug(debugLogger, "Se envio a nucleo la cadena: |%s|.", texto);


	loggearFinDePrimitiva("Imprimir texto");
}


/**
 * Directiva 12 ^
 */
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempoUsoDispositivo) {
	log_info(activeLogger,"Informar a nucleo que el programa quiere usar |%s| durante |%d| unidades de tiempo",
			dispositivo, tiempoUsoDispositivo);

	enviarHeader(nucleo,HeaderEntradaSalida);

	enviarLargoYString(nucleo,dispositivo);				//envio la cadena
// 	QUIEN HIZO ESTO QUERIA MATARME DEL DISGUSTO

//	char* time = intToChar(tiempoUsoDispositivo);							//envio el tiempo
//	send_w(nucleo,time,strlen(time));
//	free(time);

	char* time = intToChar4(tiempoUsoDispositivo);
	send_w(nucleo,time,sizeof(int));
	free(time);

	loggearFinDePrimitiva("Entrada-Salida");
}


/**
 * Directiva 13
 */
void wait_semaforo(t_nombre_semaforo identificador_semaforo) {
	log_info(activeLogger, "Comunicar nucleo de hacer wait con semaforo: |%s|",
			identificador_semaforo);

	enviarHeader(nucleo,HeaderWait);

	enviarLargoYString(nucleo,identificador_semaforo);
	//Si el proceso no pudiese seguir, nucleo al bloquearlo lo para con un header enviado a procesarHeader.

	loggearFinDePrimitiva("wait");
}


/**
 * Directiva 14
 */
void signal_semaforo(t_nombre_semaforo identificador_semaforo) {
	log_info(activeLogger,"Comunicar nucleo de hacer signal con semaforo: |%s|",identificador_semaforo);

	enviarHeader(nucleo,HeaderSignal);

	enviarLargoYString(nucleo,identificador_semaforo);


	loggearFinDePrimitiva("Signal");
}

/* ------ Funciones para usar con el parser ----- */
void inicializar_primitivas() {
	log_info(bgLogger, "Inicializando primitivas...");
	funciones.AnSISOP_definirVariable = &definir_variable;
	funciones.AnSISOP_obtenerPosicionVariable = &obtener_posicion_de;
	funciones.AnSISOP_dereferenciar = &dereferenciar;
	funciones.AnSISOP_asignar = &asignar;
	funciones.AnSISOP_obtenerValorCompartida = &obtener_valor_compartida;
	funciones.AnSISOP_asignarValorCompartida = &asignar_valor_compartida;
	funciones.AnSISOP_irAlLabel = &irAlLabel;
	funciones.AnSISOP_imprimir = &imprimir_variable;
	funciones.AnSISOP_imprimirTexto = &imprimir_texto;
	funciones.AnSISOP_llamarConRetorno = &llamar_con_retorno;
	funciones.AnSISOP_retornar = &retornar;
	funciones.AnSISOP_entradaSalida = &entrada_salida;
	funcionesKernel.AnSISOP_wait = &wait_semaforo;
	funcionesKernel.AnSISOP_signal = &signal_semaforo;
	log_info(activeLogger, "Primitivas Inicializadas.");
}
