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
#include <string.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/log.h"

// VARIABLES GLOBALES
int cliente_cpu; //cpu es cliente del nucleo
int cliente_umc; //cpu es cliente de umc

struct sockaddr_in direccionNucleo;   //direccion del nucleo
struct sockaddr_in direccionUmc;	  //direccion umc

t_log *activeLogger, *bgLogger;


int getHandshake(int cli)
{
	char* handshake = recv_nowait_ws(cli,1);
	return charToInt(handshake);
}

void conectar_nucleo(){
	direccionNucleo = crearDireccionParaCliente(8088);
	cliente_cpu = socket_w();
	connect_w(cliente_cpu,&direccionNucleo); //conecto cpu a la direccion 'direccionNucleo'

	log_info(activeLogger,"Éxito al conectar con NUCLEO!!");

}

void hacer_handshake_nucleo(){
	char* hand = string_from_format("%c%c",HeaderHandshake,SOYCPU);
	send_w(cliente_cpu,hand,sizeof(strlen(hand)));

	if(getHandshake(cliente_cpu)!= SOYNUCLEO){
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

	log_info(activeLogger,"Éxito al conectar con UMC!!");
}

void hacer_handshake_umc(){
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYCPU);
	send_w(cliente_cpu,hand,2);

	if(getHandshake(cliente_umc)!= SOYUMC){
		perror("Se esperaba que CPU se conecte con UMC.");
	}
	else{
			log_info(activeLogger,"Exito al hacer handshake");
		}
}


void hacer_handshake(int clienteid,handshake_t id){

	char *hand = string_from_format("%c%c",HeaderHandshake,id);
	send_w(clienteid,hand,2);

	if(getHandshake(clienteid)!= id){
		perror("Handshake incorrecto");
	}
	else{
			log_info(activeLogger,"Exito al hacer handshake");
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
		log_error(activeLogger,"Segunda vez que se recibe un headerHandshake acá.");
		exit(EXIT_FAILURE);
		break;
	case HeaderPCB:
		procesarPCB(); //inicio el proceso de aumentar el PC, pedir UMC sentencia...
	    break;
	default:
		log_error(activeLogger,"Llego cualquier cosa.");
		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

void esperar_programas(){
	log_debug(bgLogger,"Esperando programas de nucleo %d.");
	char* header;
	while(1){
		header = recv_waitall_ws(cliente_cpu,sizeof(char));
		procesarHeader(header);    //TODO implementar - nuevos headers?
		free(header);
	}
}

void pedir_sentencia(){
	//pedir al UMC la proxima sentencia a ejecutar
}

void procesarPCB();
void parsear();

int main()
{

	crearLogs(string_from_format("CPU_%d",getpid()),"CPU");
	log_info(activeLogger,"Soy CPU de process ID %d.", getpid());

	//conectarse a nucleo
	conectar_nucleo();
	hacer_handshake_nucleo();
	//TODO corregir error al conectar con nucleo

	//conectarse a umc
	conectar_umc();
	hacer_handshake_umc();

	//CPU se pone a esperar que nucleo le envie PCB
	esperar_programas();

	/*incrementar_pcb();
	 *
	 *solicitar_sentencia();
	 *
	 *parsear_sentencia();
	 */

	//Ejecutar

	//Actualizar en UMC

	//Actualizar Program Counter

	//Notificar fin de quantum
	destruirLogs(); //TODO cambiar de lugar

	return 0;
}
