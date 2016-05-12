/*
 * cpu.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <commons/string.h>
#include <commons/log.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <string.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"
#include "serializacion.h"

/*------------Macros--------------*/
#define DEBUG_IGNORE_UMC true


/*------------Variables Globales--------------*/
int cliente_nucleo; //cpu es cliente del nucleo
int cliente_umc; //cpu es cliente de umc

struct sockaddr_in direccionNucleo;   //direccion del nucleo
struct sockaddr_in direccionUmc;	  //dbireccion umc

AnSISOP_funciones funciones;		//funciones de AnSISOP
AnSISOP_kernel funcionesKernel;		// funciones kernel de AnSISOP

t_PCB* pcbAuxiliar;

/*------------Declaracion de funciones--------------*/
void procesarHeader(char*);
t_PCB procesarPCB();
void esperar_programas();
void pedir_sentencia();
void parsear();
void obtenerPCB();
void esperar_sentencia();

/*--------FUNCIONES----------*/
//cambiar el valor de retorno a t_puntero
void definir_variable(t_nombre_variable variable){
	printf("Definir la variable %c \n",variable);
	//return variable;
}

//cambiar valor de retorno a t_puntero
void obtener_posicion_de(t_nombre_variable variable){
	printf("Obtener posicion de %c \n",variable);
	//return variable;
}

//cambiar valor de retorno a t_valor_variable
void dereferenciar(t_puntero direccion){
	printf("Dereferenciar %d y su valor es:  \n",direccion);
	//return &direccion;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	printf("Asignando en %d el valor %d\n", direccion_variable,valor);
}

//cambiar valor de retorno a t_valor_variable
void obtener_valor_compartida(t_nombre_compartida variable){
	printf("Obtener valor de variable compartida %s y es: %d \n",variable,*variable); // no seria el valor real

}

//cambiar valor de retorno a t_valor_variable
void asignar_valor_compartida(t_nombre_compartida variable, t_valor_variable valor){
	printf("Asignar el valor %d a la variable compartida %s \n",valor,variable);
}

//cambiar valor de retorno a t_puntero_instruccion
void irAlLaber(t_nombre_etiqueta etiqueta){
	printf("Obtener puntero de %s",etiqueta);
}

//cambiar valor de retorno a t_puntero_instruccion
void llamar_sin_retorno(t_nombre_etiqueta etiqueta){
	printf("Llamar a funcion %s\n", etiqueta);
}

//cambiar valor de retorno a t_puntero_instruccion
void retornar(t_valor_variable variable){
	printf("Cambiar entorno actual usando el PC de %d \n",variable);
}

void imprimir(t_valor_variable valor){
	printf("Imprimir %d\n",valor);
	//header enviar valor de variable
	send_w(cliente_nucleo,intToChar(valor),sizeof(t_valor_variable));
}

void imprimir_texto(char* texto){
	int size = strlen(texto);
	log_debug(activeLogger, "Se envio a nucleo la cadena: %s", texto);
	send_w(cliente_nucleo, headerToMSG(HeaderImprimirTextoNucleo), 1);
	send_w(cliente_nucleo, intToChar4(size), sizeof(int));
	send_w(cliente_nucleo, texto, size); 		//envio a nucleo la cadena a imprimir
	// ??? free(texto); //como no se que onda lo que hace la blbioteca, no se si tire segment fault al hacer free. Una vez q este todoo andando probar hacer free aca
}

//cambiar valor de retorno a int
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo){
	printf("Informar a nucleo que el programa quiere usar '%s' durante %d unidades de tiempo\n",dispositivo,tiempo);
}

void wait(t_nombre_semaforo identificador_semaforo){
	printf("Comunicar nucleo de hacer wait con semaforo: %s", identificador_semaforo);
	//header hace wait
	send_w(cliente_nucleo,identificador_semaforo, sizeof(identificador_semaforo));
}


void signal(t_nombre_semaforo identificador_semaforo){
	printf("Comunicar nucleo de hacer signal con semaforo: %s", identificador_semaforo);
	//header hace signal
	send_w(cliente_nucleo,identificador_semaforo, sizeof(identificador_semaforo));
}

void inicializar_primitivas(){

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
}

void parsear(char* const sentencia){
	printf("Ejecutando: %s\n",sentencia);
	analizadorLinea(sentencia,&funciones,&funcionesKernel);
}

/*--------Funciones----------*/

int getHandshake(int cli){
	char* handshake = recv_nowait_ws(cli,1);
	return charToInt(handshake);
}

void conectar_nucleo(){
	direccionNucleo = crearDireccionParaCliente(8088,"127.0.0.1"); //TODO cambiar ip
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
		log_info(activeLogger,"Exito al hacer handshake");
	}
}

void conectar_umc(){
	direccionUmc = crearDireccionParaCliente(8081,"127.0.0.1"); //TODO cambiar ip
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
			log_info(activeLogger,"Exito al hacer handshake");
		}
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
		log_error(activeLogger,"Segunda vez que se recibe un headerHandshake acÃ¡.");
		exit(EXIT_FAILURE);
		break;

	case HeaderPCB:
		obtenerPCB(); //inicio el proceso de aumentar el PC, pedir UMC sentencia...
	    break;

	case HeaderSentencia:
		//obtener_y_parsear();
		break;
	default:
		log_error(activeLogger,"Llego cualquier cosa.");
		log_error(activeLogger,"Llego el header numero %d y no hay una acciÃ³n definida para Ã©l.",charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

void pedir_sentencia(){
	//pedir al UMC la proxima sentencia a ejecutar
	char* solic = string_from_format("%c",HeaderSolicitudSentencia);
	send_w(cliente_umc,solic,sizeof(strlen(solic)));
}

void esperar_sentencia(){
	char* header = recv_waitall_ws(cliente_nucleo,sizeof(char));
	procesarHeader(header);
	free(header);
}

void obtenerPCB(){
	//funcion que recibe el pcb
	pedir_sentencia();

	esperar_sentencia();
}

void obtener_y_parsear(void){
	char tamanioSentencia;
	read(cliente_umc, &tamanioSentencia, 1);

	char* sentencia = recv_waitall_ws(cliente_umc,tamanioSentencia);
	//parsear(sentencia);  arreglar! rompe el parser
	free(sentencia);
}

t_PCB procesarPCB(t_PCB pcb){
	t_PCB nuevoPCB;	//incrementar registro
	nuevoPCB = pcb;
	nuevoPCB.PC++;
	return nuevoPCB;
}

void serializar_PCB(char* res, t_PCB* pcb){		//terminar!

	string_append(&res,intToChar4(pcb->PC));		//pongo el PC
	string_append(&res,intToChar4(pcb->PID));		//pongo el PID
	//serializar paginas de codigo
	string_append(&res,intToChar4(pcb->cantidad_paginas));	//pongo la cantidad de paginas
	//serializar indice etiquetas
	//serializar indice stack

}

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

void inicializar(){
	pcbAuxiliar = malloc(sizeof(t_PCB));
	crearLogs(string_from_format("cpu_%d",getpid()),"CPU");
	log_info(activeLogger,"Soy CPU de process ID %d.", getpid());
	inicializar_primitivas();
}

void finalizar(){
	destruirLogs();
	free(pcbAuxiliar);
	liberar_primitivas();
}

int main()
{
	inicializar();

	//conectarse a umc
	establecerConexionConUMC();


	//conectarse a nucleo
	conectar_nucleo();
	hacer_handshake_nucleo();

	//CPU se pone a esperar que nucleo le envie PCB
	esperar_programas();

	finalizar();
	return EXIT_SUCCESS;
}
