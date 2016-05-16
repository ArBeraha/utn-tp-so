/*
 * cpu.c
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "cpu.h"

/*------------Declaracion de funciones--------------*/
void procesarHeader(char*);
//t_PCB procesarPCB();
void esperar_programas();
void pedir_sentencia();
void parsear();
void obtenerPCB();
void esperar_sentencia();
void obtener_y_parsear();

/*----- Operaciones sobre el PC y avisos por quantum -----*/
void informarInstruccionTerminada(){
	// Le aviso a nucleo que termino una instruccion, para que calcule cuanto quantum le queda al proceso ansisop.
	send_w(cliente_nucleo,headerToMSG(headerTermineInstruccion),1);
}
void setearPC(t_PCB* pcb,int pc){
	pcb->PC=pc;
}
void incrementarPC(t_PCB* pcb){
	pcb->PC++;
}
void instruccionTerminada(char* instr){
	log_debug(activeLogger, "%s finalizó OK.", instr);
}

/*--------FUNCIONES----------*/
//cambiar el valor de retorno a t_puntero
void definir_variable(t_nombre_variable variable){
	log_info(activeLogger,"Definir la variable |%c|.",variable);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Definir_variable");
	//return variable;
}

//cambiar valor de retorno a t_puntero
void obtener_posicion_de(t_nombre_variable variable){
	log_info(activeLogger,"Obtener posicion de |%c|.",variable);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("obtener_posicion_de");
	//return variable;
}

t_valor_variable dereferenciar(t_puntero direccion){		// TODO terminar - Pido a UMC el valor de la variable de direccion
	t_valor_variable valor;
	log_info(activeLogger,"Dereferenciar |%d| y su valor es:  ",direccion);

	send_w(cliente_umc, headerToMSG(HeaderPedirValorVariable), 1);
	send_w(cliente_umc,intToChar(direccion),sizeof(t_puntero)); // t_puntero y t_valor_variable es entero de 32 bits -- deberia serializar?

	char* msgSize = recv_waitall_ws(cliente_umc, sizeof(int));
	int size = char4ToInt(msgSize);
	char* res = recv_waitall_ws(cliente_umc,size); //recibo el valor de UMC

	valor = charToInt(res);
	free(msgSize);
	free(res);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Dereferenciar");
	return valor;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){	//TODO terminar
	log_info(activeLogger,"Asignando en |%d| el valor |%d|", direccion_variable,valor);
	send_w(cliente_umc,headerToMSG(HeaderAsignarValor), 1);
	send_w(cliente_umc,intToChar(direccion_variable),sizeof(t_puntero)); //envio la direccion de la variable
	send_w(cliente_umc,intToChar(valor),sizeof(t_valor_variable));		//envio el valor de la variable
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Asignar");
}


t_valor_variable obtener_valor_compartida(t_nombre_compartida variable){ 				// Pido a Nucleo el valor de la variable
	log_info(activeLogger,"Obtener valor de variable compartida %s y es: |%d|.",variable,*variable); // no seria el valor real
	t_valor_variable valor;

	send_w(cliente_nucleo, headerToMSG(HeaderPedirValorVariableCompartida), 1);
	send_w(cliente_nucleo,variable,sizeof(t_nombre_compartida));

	char* msgSize = recv_waitall_ws(cliente_nucleo, sizeof(int));
	int size = char4ToInt(msgSize);
	char* res = recv_waitall_ws(cliente_nucleo,size);

	valor = charToInt(res);
	free(msgSize);
	free(res);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Obtener_valor_compartida");
	return valor;
}

t_valor_variable asignar_valor_compartida(t_nombre_compartida variable, t_valor_variable valor){
	log_info(activeLogger,"Asignar el valor |%d| a la variable compartida |%s|.",valor,variable);

	send_w(cliente_nucleo, headerToMSG(HeaderAsignarValorVariableCompartida), 1);		//envio el header

	send_w(cliente_nucleo,variable,sizeof(t_nombre_compartida));						//envio el nombre de la variable

	char* valor_envio = intToChar4(valor);
	send_w(cliente_nucleo,valor_envio,sizeof(int));								//envio el valor

	//TODO esperar a que nucleo informe la asignacion, para no usar un valor antiguo.
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("asignar_valor_compartida");
	return valor;
}

//cambiar valor de retorno a t_puntero_instruccion
void irAlLaber(t_nombre_etiqueta etiqueta){
	log_info(activeLogger,"Obtener puntero de |%s|.",etiqueta);
	int posicionEtiqueta=0; // TODO acá va la de la etiqueta
	setearPC(pcbActual,posicionEtiqueta);
	informarInstruccionTerminada();
	instruccionTerminada("ir_al_laber");
}

//cambiar valor de retorno a t_puntero_instruccion
void llamar_sin_retorno(t_nombre_etiqueta nombreFuncion){
	log_info(activeLogger,"Llamar a funcion |%s|.", nombreFuncion);
	int posicionFuncion=0; // TODO acá va la de la funcion
	setearPC(pcbActual,posicionFuncion);
	informarInstruccionTerminada();
	instruccionTerminada("Llamar_sin_retorno");
}

//cambiar valor de retorno a t_puntero_instruccion
void retornar(t_valor_variable variable){
	log_info(activeLogger,"Cambiar entorno actual usando el PC de |%d| a |%d|.",pcbActual->PC,variable);
	setearPC(pcbActual,(int)variable);
	informarInstruccionTerminada();
	instruccionTerminada("Retornar");
}

void imprimir(t_valor_variable valor){
	log_info(activeLogger,"Imprimir |%d|\n",valor);
	//header enviar valor de variable
	send_w(cliente_nucleo,intToChar(valor),sizeof(t_valor_variable));
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir");
}

void imprimir_texto(char* texto){
	int size = strlen(texto)+1; // El strlen no cuenta el \0. strlen("hola\0") = 4.
	log_debug(activeLogger, "Enviando a nucleo la cadena: |%s|...", texto);
	send_w(cliente_nucleo, headerToMSG(HeaderImprimirTextoNucleo), 1);
	send_w(cliente_nucleo, intToChar4(size), sizeof(int));
	send_w(cliente_nucleo, texto, size); 		//envio a nucleo la cadena a imprimir
	log_debug(activeLogger, "Se envio a nucleo la cadena: |%s|.", texto);
	// TODO ??? free(texto); //como no se que onda lo que hace la blbioteca, no se si tire segment fault al hacer free. Una vez q este todoo andando probar hacer free aca
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Imprimir texto");
}

//cambiar valor de retorno a int
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo){
	log_info(activeLogger,"Informar a nucleo que el programa quiere usar |%s| durante |%d| unidades de tiempo",dispositivo,tiempo);
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("Entrada-Salida");
}

void wait(t_nombre_semaforo identificador_semaforo){
	log_info(activeLogger,"Comunicar nucleo de hacer wait con semaforo: |%s|", identificador_semaforo);
	send_w(cliente_nucleo,headerToMSG(HeaderWait),1);
	send_w(cliente_nucleo,identificador_semaforo, sizeof(identificador_semaforo));
	incrementarPC(pcbActual);
	informarInstruccionTerminada();
	instruccionTerminada("wait");
}


void signal(t_nombre_semaforo identificador_semaforo){
	log_info(activeLogger,"Comunicar nucleo de hacer signal con semaforo: |%s|", identificador_semaforo);
	send_w(cliente_nucleo,headerToMSG(HeaderSignal),1);
	send_w(cliente_nucleo,identificador_semaforo, sizeof(identificador_semaforo));
	incrementarPC(pcbActual);
	instruccionTerminada("Signal");
}


/* ------ Funciones para usar con el parser ----- */
void inicializar_primitivas(){
	log_info(bgLogger,"Inicianlizando primitivas...");
	funciones.AnSISOP_definirVariable = &definir_variable;
	funciones.AnSISOP_obtenerPosicionVariable = &obtener_posicion_de;
	funciones.AnSISOP_dereferenciar= &dereferenciar;
	funciones.AnSISOP_asignar= &asignar;
	funciones.AnSISOP_obtenerValorCompartida= &obtener_valor_compartida;
	funciones.AnSISOP_asignarValorCompartida= &asignar_valor_compartida;
	funciones.AnSISOP_irAlLabel= &irAlLaber;
	funciones.AnSISOP_imprimir = &imprimir;
	funciones.AnSISOP_imprimirTexto= &imprimir_texto;
	funciones.AnSISOP_llamarSinRetorno = &llamar_sin_retorno;
	funciones.AnSISOP_retornar=&retornar;
	funciones.AnSISOP_entradaSalida=&entrada_salida;
	funcionesKernel.AnSISOP_wait= &wait;
	funcionesKernel.AnSISOP_signal=&signal;
	log_info(activeLogger,"Primitivas Inicializadas");
}

void liberar_primitivas(){
	log_debug(bgLogger, "Liberando primitivas...");
	free(funciones.AnSISOP_definirVariable );
	free(funciones.AnSISOP_obtenerPosicionVariable  );
	free(funciones.AnSISOP_dereferenciar );
	free(funciones.AnSISOP_asignar );
	free(funciones.AnSISOP_obtenerValorCompartida );
	free(funciones.AnSISOP_asignarValorCompartida );
	free(funciones.AnSISOP_irAlLabel );
	free(funciones.AnSISOP_imprimir);
	free(funciones.AnSISOP_imprimirTexto);
	free(funciones.AnSISOP_llamarSinRetorno);
	free(funciones.AnSISOP_retornar);
	free(funciones.AnSISOP_entradaSalida);
	free(funcionesKernel.AnSISOP_wait);
	free(funcionesKernel.AnSISOP_signal);
	log_debug(bgLogger, "Primitivas liberadas...");
}

void parsear(char* const sentencia){
	log_info(activeLogger,"Ejecutando: %s\n",sentencia);
	analizadorLinea(sentencia,&funciones,&funcionesKernel);
}

/*--------Funciones----------*/

int getHandshake(int cli){
	char* handshake = recv_nowait_ws(cli,1);
	return charToInt(handshake);
}

void conectar_nucleo(){
	direccionNucleo = crearDireccionParaCliente(config.puertoNucleo,config.ipNucleo);
	cliente_nucleo = socket_w();
	connect_w(cliente_nucleo,&direccionNucleo); //conecto cpu a la direccion 'direccionNucleo'
	log_info(activeLogger,"Exito al conectar con NUCLEO!!");
}

void hacer_handshake_nucleo(){
	char* hand = string_from_format("%c%c",HeaderHandshake,SOYCPU);
	send_w(cliente_nucleo,hand,2);

	if(getHandshake(cliente_nucleo)!= SOYNUCLEO){
		perror("Se esperaba que CPU se conecte con el nucleo.");
	}
	else{
		log_info(bgLogger,"Exito al hacer handshake con nucleo.");
	}
}

void conectar_umc(){
	direccionUmc = crearDireccionParaCliente(config.puertoUMC,config.ipUMC);
	cliente_umc = socket_w();
	connect_w(cliente_umc,&direccionUmc); //conecto cpu a la direccion 'direccionUmc'
	log_info(activeLogger,"Exito al conectar con UMC!!");
}

void hacer_handshake_umc(){
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYCPU);
	send_w(cliente_umc,hand,2);

	if(getHandshake(cliente_umc)!= SOYUMC){
		perror("Se esperaba que CPU se conecte con UMC.");
	}
	else{
			log_info(bgLogger,"Exito al hacer handshake con UMC.");
		}
}

void pedir_tamanio_paginas(){
	send_w(cliente_umc,headerToMSG(HeaderTamanioPagina),1); //le pido a umc el tamanio de las paginas
	char* tamanio = recv_nowait_ws(cliente_umc, sizeof(int)); //recibo el tamanio de las paginas
	tamanioPaginas = char4ToInt(tamanio);
	log_debug(activeLogger, "El tamaño de paginas es: %d", tamanioPaginas);
	free(tamanio);
}


void esperar_programas(){
	log_debug(bgLogger,"Esperando programas de nucleo %d.");
	char* header;
	while(1){
		header = recv_waitall_ws(cliente_nucleo,1);
		procesarHeader(header);
		free(header);
	}
}

void procesarHeader(char *header){
	// Segun el protocolo procesamos el header del mensaje recibido
	log_debug(bgLogger,"Llego un mensaje con header %d.",charToInt(header));

	switch(charToInt(header)) {

	case HeaderError:
		log_error(activeLogger,"Header de Error.");
		break;

	case HeaderHandshake:
		log_error(activeLogger,"Segunda vez que se recibe un headerHandshake acá!.");
		exit(EXIT_FAILURE);
		break;

	case HeaderPCB:
		obtenerPCB(); //inicio el proceso de aumentar el PC, pedir UMC sentencia...
	    break;

	case HeaderSentencia:
		obtener_y_parsear();
		break;
	default:
		log_error(activeLogger,"Llego cualquier cosa.");
		log_error(activeLogger,"Llego el header numero %d y no hay una accion definida para el.",charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

int longitud_sentencia(t_sentencia* sentencia){
	return sentencia->offset_fin - sentencia->offset_inicio;
}

int obtener_offset_relativo(t_sentencia* fuente, t_sentencia* destino){
	int offsetInicio = fuente->offset_inicio;
	int numeroPagina = (int) (offsetInicio / tamanioPaginas) ;  //obtengo el numero de pagina
	int offsetRelativo = (int) offsetInicio % tamanioPaginas;			//obtengo el offset relativo

	int longitud = longitud_sentencia(fuente);

	destino->offset_inicio = offsetRelativo;
	destino->offset_fin = offsetRelativo + longitud;

	return numeroPagina;
}

int cantidad_paginas_ocupa(t_sentencia* sentencia){ //precondicion: el offset debe ser el relativo
	int cant= (int)longitud_sentencia(sentencia)/tamanioPaginas;
	return cant + 1;
}

int queda_espacio_en_pagina(t_sentencia* sentencia){ //precondicion: el offset debe ser el relativo
	int longitud = longitud_sentencia(sentencia);
	int desp = sentencia->offset_inicio + longitud;
	return tamanioPaginas - desp;;
}

void enviar_pagina(int pagina){				//envio la pagina
	char* pag = intToChar4(pagina);
	send_w(cliente_umc,pag,sizeof(int));
	free(pag);
}

void enviar_longitud(int longitud){		//envio la longitud de la sentencia
	char* longi =  intToChar4(longitud);
	send_w(cliente_umc,longi,sizeof(int));
	free(longi);
}

void enviar_sentencia(t_sentencia* sentencia){
	char* envio = string_new();
	int size = serializar_sentencia(envio,sentencia);
	send_w(cliente_umc,envio,size);
}

void pedir_sentencia(){	//pedir al UMC la proxima sentencia a ejecutar
	int entrada = pcbActual->PC;   				//obtengo la entrada de la instruccion a ejecutar

	t_sentencia* sentenciaActual = list_get(pcbActual->indice_codigo,entrada);		//obtengo el offset de la sentencia
	t_sentencia* sentenciaParaUMC = malloc(sizeof(t_sentencia));

	int pagina = obtener_offset_relativo(sentenciaActual,sentenciaParaUMC);			//obtengo el offset relativo

	send_w(cliente_umc, headerToMSG(HeaderSolicitudSentencia), 1);    			//envio el header

	char* sentencia = string_new();

	int i = 0;
	int longitud_restante = longitud_sentencia(sentenciaParaUMC);
	int cantidad_pags =cantidad_paginas_ocupa(sentenciaParaUMC);
	printf("La instruccion ocupa %d paginas\n", cantidad_pags);

	while(i< cantidad_pags ){			//me fijo si ocupa mas de una pagina

		printf("envie la pag: %d\n",pagina + i);

		if(longitud_restante > tamanioPaginas){						//si me paso de la pagina, acorto el offset fin
			longitud_restante = longitud_restante - tamanioPaginas;
			sentenciaParaUMC->offset_fin = tamanioPaginas;
			printf("offset: %d,pagina: %d, size: %d\n",sentenciaParaUMC->offset_inicio, pagina + i, tamanioPaginas);
			sentenciaParaUMC->offset_inicio = 0;
		}
		else{													//si no me paso, sigo igual y terminaria el while
			sentenciaParaUMC->offset_fin = longitud_restante;
			printf("offset: %d,pagina: %d, size: %d\n",sentenciaParaUMC->offset_inicio, pagina + i, longitud_restante);

		}
		enviar_pagina(pagina);
		enviar_longitud(longitud_restante);
		enviar_sentencia(sentenciaParaUMC);

		i++;
	}
	free(sentencia);
	free(sentenciaActual);
	free(sentenciaParaUMC);
}

void esperar_sentencia(){
	char* header = recv_waitall_ws(cliente_nucleo,sizeof(char));
	procesarHeader(header);
	free(header);
}

void obtenerPCB(){
	pedir_sentencia();
	esperar_sentencia();
}

void obtener_y_parsear(){
	char tamanioSentencia;
	read(cliente_umc, &tamanioSentencia, 1);

	char* sentencia = recv_waitall_ws(cliente_umc,tamanioSentencia);
	parsear(sentencia);
	free(sentencia);
}

// @Emi: retornaria un PCB nuevo con el PC actualizado, manteniendo el viejo. Como no se si la necesitas para algo, la dejo acá
//t_PCB procesarPCB(t_PCB pcb){
//	t_PCB nuevoPCB;	//incrementar registro
//	nuevoPCB = pcb;
//	nuevoPCB.PC++;
//	return nuevoPCB;
//}
// @emi las comento porque sino me dice que redefinen las de ari y no compila.
//void serializar_PCB(char* res, t_PCB* pcb){		//terminar!
//
//	string_append(&res,intToChar4(pcb->PC));		//pongo el PC
//	string_append(&res,intToChar4(pcb->PID));		//pongo el PID
//	//serializar paginas de codigo
//	string_append(&res,intToChar4(pcb->cantidad_paginas));	//pongo la cantidad de paginas
//	//serializar indice etiquetas
//	//serializar indice stack
//
//}
//void deserializar_PCB(char* mensaje, t_PCB* pcb){
//	//t_PCB* newPCB = malloc(24);
//
//	pcb->PC = atoi(string_substring(mensaje,0,3));	//pongo el PC
//	pcb->PID = atoi(string_substring(mensaje,4,8));
//
//	//deserializar indices de codigo
//
//	pcb->cantidad_paginas = atoi(string_substring(mensaje,13,16));
//
//}

// ***** Funciones de conexiones ***** //
void warnDebug() {
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_info(activeLogger,
			"Para ingresar manualmente un archivo: Cambiar true por false en cpu.c -> #define DEBUG_IGNORE_UMC, y despues recompilar.");
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
}
void establecerConexionConUMC(){
	if(!DEBUG_IGNORE_UMC){
			conectar_umc();
			hacer_handshake_umc();
		}
		else{
			warnDebug();
		}
}
void establecerConexionConNucleo(){
	conectar_nucleo();
	hacer_handshake_nucleo();
}

// ***** Funciones de inicializacion y finalizacion ***** //
void cargarConfig(){
	t_config* configCPU;
	configCPU = config_create("cpu.cfg");
	config.puertoNucleo = config_get_int_value(configCPU, "PUERTO_NUCLEO");
	config.ipNucleo = config_get_string_value(configCPU, "IP_NUCLEO");
	config.puertoUMC = config_get_int_value(configCPU, "PUERTP_UMC");
	config.ipUMC = config_get_string_value(configCPU, "IP_UMC");
}
void inicializar(){
	cargarConfig();
	pcbActual = malloc(sizeof(t_PCB));
	crearLogs(string_from_format("cpu_%d",getpid()),"CPU");
	log_info(activeLogger,"Soy CPU de process ID %d.", getpid());
	inicializar_primitivas();
}
void finalizar(){
	destruirLogs(); //fixme: no hay que usar las funciones que nos dan ellos?
	pcb_destroy(pcbActual);
	liberar_primitivas();
}

int main()
{
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
