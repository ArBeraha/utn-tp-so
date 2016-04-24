/*
 * consola.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <string.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include <commons/log.h>
#include <unistd.h>

#define PATHSIZE 2048
#define DEBUG true

typedef int ansisop_var_t; //TODO cambiar esto si es necesario!
int cliente;
t_log* activeLogger,* bgLogger;
FILE* programa;

void crearLogs()
{
	activeLogger = log_create(string_from_format("consola_%d.log",getpid()),"Consola",true,LOG_LEVEL_INFO);
	bgLogger = log_create(string_from_format("consola_%d.log",getpid()),"Consola",false,LOG_LEVEL_DEBUG);
}

void destruirLogs()
{
	log_destroy(activeLogger);
	log_destroy(bgLogger);
}

void sacarSaltoDeLinea(char* texto) // TODO testear! Hice esta funcion desde el navegador xD
{
	//Lee y termina por \n y \0, entonces si hay un \n lo piso con \0, y si hay un \0 lo piso con \0 (lease, no hago nada xD)
	texto[strcspn(buffer, "\n")]='\0';
}
void imprimirVariable()
{
	char* msgValue = recv_waitall_ws(cliente,sizeof(ansisop_var_t));
	int value = charToInt(msgValue);
	char* name = recv_waitall_ws(cliente,sizeof(char));
	sacarSaltoDeLinea(name);
	log_info(activeLogger,"Variable %s: %d.",name,value);
	free(msgValue);
	free(name);
}

void imprimirTexto()
{
	char* msgSize = recv_waitall_ws(cliente,sizeof(int));
	int size = charToInt(msgSize);
	char* texto = recv_waitall_ws(cliente,size);
	sacarSaltoDeLinea(name);
	log_info(activeLogger,"%s",texto);
	free (msgSize);
	free (texto);
}

void conectarANucleo()
{
	direccion = crearDireccionParaCliente(8080);
	cliente = socket_w();
	connect_w(cliente, &direccion);
}

void finalizar()
{
	fclose(programa);
	log_info(activeLogger,"Fin exitoso.");
	destruirLogs();
	// close(cliente); // TODO se hace aca o lo maneja el nucleo con "quitarCliente" al finalizar una consola en forma normal?"
	exit(EXIT_SUCCESS); //Un return desde main hace un exit, asi que es lo mismo hacerlo aca como exit!
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

	case HeaderConsolaImprimi:
		imprimirVariable();
		break;

	case HeaderConsolaImprimiTexto:
		imprimirTexto();
		break;

	case HeaderConsolaFinalizarNormalmente:
		finalizar();
		break;

	default:
		log_error(activeLogger,"Llego cualquier cosa.");
		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,1);
	return charToInt(handshake);
}

void handshakear()
{
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYCONSOLA);
	send_w(cliente, hand, 2);

	log_debug(bgLogger,"Consola handshakeo.");
	if(getHandshake()!=SOYNUCLEO)
	{
		perror("Se esperaba que la consola se conecte con el nucleo.");
	}
	else
		log_debug(bgLogger,"Consola recibio handshake de Nucleo.");
}

void escucharPedidos()
{
	char* header;
	while(true)
		{
			header = recv_waitall_ws(cliente,sizeof(char));
			procesarHeader(header);
			free(header);
		}
}

void realizarConexion()
{
	conectarANucleo();
	log_info(activeLogger,"Conexion al nucleo correcta :).");
	handshakear();
	log_info(activeLogger,"Handshake finalizado exitosamente.");
	log_debug(bgLogger,"Esperando algo para imprimir en pantalla.");
}

void warnDebug()
{
	log_warning(activeLogger,"--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_info(activeLogger,"Para ingresar manualmente un archivo: Cambiar true por false en consola.c -> #define DEBUG, y despues recompilar.", getpid());
	log_warning(activeLogger,"--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
}

void cargarArchivoEnMemoria()
{
	// TODO entrega 2
}
void enviarContenidoDelArchivo()
{
	//TODO entrega 2
}

int main(int argc, char* argv[])
{
	//TODO lo de pasar los handshakes como un byte por meter el numero en un caracter, va barbaro! total son 5
	// ahora... con los headers que hacemos? cuando lleguemos al header 10 explota todo.
	crearLogs();
	char* path = malloc(PATHSIZE);
	log_info(activeLogger,"Soy consola de process ID %d.", getpid());

	if(DEBUG){
		warnDebug();
	}
	else{
		if(argc==1){
				printf("Ingresar archivo ansisop: ");
				log_debug(bgLogger,"Se silicito ingresar un archivo ansisop.");
				scanf("%s",path);
			}
			else if(argc==2){
				 path = argv[1];
			}
			else{
				log_error(activeLogger,"Muchos parametros.");
				log_info(activeLogger,"No poner parametros o poner solo el nombre del archivo a abrir");
				exit(EXIT_FAILURE);
			}
	}


	if(DEBUG){
		log_debug(activeLogger,"Se va a abrir:%s","facil.ansisop");
		programa = fopen("facil.ansisop","r");
	}
	else{
		programa = fopen(path,"r");
	}

	if(programa==NULL){
		perror("No se encontro el archivo.");
	}
	free(path);

	// Me conecto al nucleo y hago el handshake
	realizarConexion();

	// si se pasa el path no funciona con los procesos corriendo en diferentes maquinas.
	cargarArchivoEnMemoria();
	enviarContenidoDelArchivo();
	
	// Escucho pedidos (de impresion) hasta que el header que llegue sea de finalizar.
	// En ese caso se finaliza.
	escucharPedidos();
	return EXIT_SUCCESS;
}

