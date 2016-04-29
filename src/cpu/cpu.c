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
#include <commons/string.h>
#include <commons/log.h>
#include <parser/parser.h>
#include <string.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/log.h"
#include "../otros/commonTypes.h"

/*------------Variables Globales--------------*/
int cliente_nucleo; //cpu es cliente del nucleo
int cliente_umc; //cpu es cliente de umc

struct sockaddr_in direccionNucleo;   //direccion del nucleo
struct sockaddr_in direccionUmc;	  //dbireccion umc

//t_log *activeLogger, *bgLogger;
//
//AnSISOP_funciones funciones;		//funciones de AnSISOP
//AnSISOP_kernel funcionesKernel;		// funciones kernel de AnSISOP

/*------------Declaracion de funciones--------------*/
void procesarHeader(char*);
void procesarPCB();
void esperar_programas();
void procesarPCB();
void pedir_sentencia();
void parsear();
void obtenerPCB();
void esperar_sentencia();

/*--------Funciones----------*/ //COMENTADO LO DEL PARSER HASTA QUE COMPILE BIEN -.-
//cambiar el valor de retorno a t_puntero
//void definir_variable(t_nombre_variable variable){
//	printf("Definir la variable %c \n",variable);
//	//return variable;
//}
//
////cambiar valor de retorno a t_puntero
//void obtener_posicion_de(t_nombre_variable variable){
//	printf("Obtener posicion de %c \n",variable);
//	//return variable;
//}
//
////cambiar valor de retorno a t_valor_variable
//void dereferenciar(t_puntero direccion){
//	printf("Dereferenciar %d y su valor es:  \n",direccion);
//	//return &direccion;
//}
//
//void asignar(t_puntero direccion_variable, t_valor_variable valor){
//	printf("Asignando en %d el valor %d\n", direccion_variable,valor);
//}
//
////cambiar valor de retorno a t_valor_variable
//void obtener_valor_compartida(t_nombre_compartida variable){
//	printf("Obtener valor de variable compartida %s y es: %d \n",variable,*variable); // no seria el valor real
//
//}
//
////cambiar valor de retorno a t_valor_variable
//void asignar_valor_compartida(t_nombre_compartida variable, t_valor_variable valor){
//	printf("Asignar el valor %d a la variable compartida %s \n",valor,variable);
//}
//
////cambiar valor de retorno a t_puntero_instruccion
//void irAlLaber(t_nombre_etiqueta etiqueta){
//	printf("Obtener puntero de %s",etiqueta);
//}
//
////cambiar valor de retorno a t_puntero_instruccion
//void llamar_sin_retorno(t_nombre_etiqueta etiqueta){
//	printf("Llamar a funcion %s\n", etiqueta);
//}
//
////cambiar valor de retorno a t_puntero_instruccion
//void retornar(t_valor_variable variable){
//	printf("Cambiar entorno actual usando el PC de %d \n",variable);
//}
//
//void imprimir(t_valor_variable valor){
//	printf("Imprimir %d\n",valor);
//}
//
//void imprimir_texto(char* texto){
//	 printf("Imprimir texto: %s",texto);
//}
//
////cambiar valor de retorno a int
//void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo){
//	printf("Informar a nucleo que el programa quiere usar '%s' durante %d unidades de tiempo\n",dispositivo,tiempo);
//}
//
////cambiar valor de retorno a int
//void wait(t_nombre_semaforo identificador_semaforo){
//	printf("Comunicar nucleo de hacer wait con semaforo: %s", identificador_semaforo);
//}
//
////cambiar valor de retorno a int
//void signal(t_nombre_semaforo identificador_semaforo){
//	printf("Comunicar nucleo de hacer signal con semaforo: %s", identificador_semaforo);
//}
//
//void inicializar_primitivas(){
//
//	funciones.AnSISOP_definirVariable = &definir_variable;
//	funciones.AnSISOP_obtenerPosicionVariable = &obtener_posicion_de;
//	funciones.AnSISOP_dereferenciar= &dereferenciar;
//	funciones.AnSISOP_asignar= &asignar;
//	funciones.AnSISOP_obtenerValorCompartida= &obtener_valor_compartida;
//	funciones.AnSISOP_asignarValorCompartida= &asignar_valor_compartida;
//	funciones.AnSISOP_irAlLabel= &irAlLaber;
//	funciones.AnSISOP_imprimir = &imprimir;
//	funciones.AnSISOP_imprimirTexto= &imprimir_texto;
//	funciones.AnSISOP_llamarSinRetorno = &llamar_sin_retorno;
//	funciones.AnSISOP_retornar=&retornar;
//	funciones.AnSISOP_entradaSalida=&entrada_salida;
//	funcionesKernel.AnSISOP_wait= &wait;
//	funcionesKernel.AnSISOP_signal=&signal;
//
//}
//
//void parsear(char* const sentencia){
//	printf("Ejecutando: %s\n",sentencia);
//	analizadorLinea(sentencia,&funciones,&funcionesKernel);
//}

/*--------Funciones----------*/

int getHandshake(int cli)
{
	char* handshake = recv_nowait_ws(cli,1);
	return charToInt(handshake);
}

void conectar_nucleo(){
	direccionNucleo = crearDireccionParaCliente(8088);
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
	direccionUmc = crearDireccionParaCliente(8081);
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
		header = recv_waitall_ws(cliente_nucleo,sizeof(char));
		procesarHeader(header);    //TODO implementar - nuevos headers?
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

void procesarPCB(t_PCB* pcb){
	pcb->PC++;	//incrementar registro
}

int main()
{
	crearLogs(string_from_format("CPU_%d",getpid()),"CPU");
	log_info(activeLogger,"Soy CPU de process ID %d.", getpid());

	//inicializar_primitivas();
	//parsear("a = b + 12");

	//conectarse a nucleo
	conectar_nucleo();
	hacer_handshake_nucleo();

	//conectarse a umc
	conectar_umc();
	hacer_handshake_umc();

	//CPU se pone a esperar que nucleo le envie PCB
	esperar_programas();


	destruirLogs(); //TODO cambiar de lugar

	return 0;
}
