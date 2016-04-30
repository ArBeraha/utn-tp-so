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
#include <commons/log.h>
#include <unistd.h>
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"

#define PATHSIZE 2048

// ********* INICIO DEBUG ******** //
#define DEBUG false
#define DEBUG_LOG_OLD_REMOVE false
// ********* FIN DEBUG ******** //

FILE* programa;
int cliente;

void sacarSaltoDeLinea(char* texto) // TODO testear! Hice esta funcion desde el navegador xD
{
	//Lee y termina por \n y \0, entonces si hay un \n lo piso con \0, y si hay un \0 lo piso con \0 (lease, no hago nada xD)
	texto[strcspn(texto, "\n")] = '\0';
}

void imprimirVariable() {
	char* msgValue = recv_waitall_ws(cliente, sizeof(ansisop_var_t));
	int value = charToInt(msgValue);
	char* name = recv_waitall_ws(cliente, sizeof(char));
	sacarSaltoDeLinea(name);
	// uso printf y logger de background solo porque es un mensaje impreso normalmente
	// y no algo del log.
	printf("Variable %s: %d.", name, value);
	log_debug(bgLogger, "Mensaje impreso: Variable %s: %d.", name, value);
	free(msgValue);
	free(name);
}

void imprimirTexto() {
	char* msgSize = recv_waitall_ws(cliente, sizeof(int));
	int size = charToInt(msgSize);
	char* texto = recv_waitall_ws(cliente, size);
	sacarSaltoDeLinea(texto);
	printf("%s\n", texto);
	// uso printf y logger de background solo porque es un mensaje impreso normalmente
	// y no algo del log.
	log_debug(bgLogger, "Mensaje impreso: %s", texto);
	free(msgSize);
	free(texto);
}

void conectarANucleo() {
	direccion = crearDireccionParaCliente(8080);
	cliente = socket_w();
	connect_w(cliente, &direccion);
}

void finalizar() {
	log_info(activeLogger, "Fin exitoso.");
	destruirLogs();
	// el fclose se hace apenas se deja de usar el archivo para poder correr 2 instancias del mismo proceso ansisop.
	// close(cliente); // TODO se hace aca o lo maneja el nucleo con "quitarCliente" al finalizar una consola en forma normal?"
	exit(EXIT_SUCCESS); //Un return desde main hace un exit, asi que es lo mismo hacerlo aca como exit!
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
				"Segunda vez que se recibe un headerHandshake acá.");
		exit(EXIT_FAILURE);
		break;

	case HeaderImprimirVariableConsola:
		imprimirVariable();
		break;

	case HeaderImprimirTextoConsola:
		imprimirTexto();
		break;

	case HeaderConsolaFinalizarNormalmente:
		finalizar();
		break;

	default:
		log_error(activeLogger, "Llego cualquier cosa.");
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}

int getHandshake() {
	char* handshake = recv_nowait_ws(cliente, 1);
	return charToInt(handshake);
}

void handshakear() {
	char *hand = string_from_format("%c%c", HeaderHandshake, SOYCONSOLA);
	send_w(cliente, hand, 2);

	log_debug(bgLogger, "Consola handshakeo.");
	if (getHandshake() != SOYNUCLEO) {
		perror("Se esperaba que la consola se conecte con el nucleo.");
	} else
		log_debug(bgLogger, "Consola recibio handshake de Nucleo.");
}

void escucharPedidos() {
	char* header;
	while (true) {
		header = recv_waitall_ws(cliente, sizeof(char));
		procesarHeader(header);
		free(header);
	}
}

void realizarConexion() {
	conectarANucleo();
	log_info(activeLogger, "Conexion al nucleo correcta :).");
	handshakear();
	log_info(activeLogger, "Handshake finalizado exitosamente.");
	log_debug(bgLogger, "Esperando algo para imprimir en pantalla.");
}

void warnDebug() {
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_info(activeLogger,
			"Para ingresar manualmente un archivo: Cambiar true por false en consola.c -> #define DEBUG, y despues recompilar.");
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
}

void cargarYEnviarArchivo() {
	log_debug(bgLogger, "Empezando la lectura del script.");
	size_t length = 0;
	int size = 0;
	char* line = NULL;
	char* contenido = string_new();
	ssize_t read = getline(&line, &length, programa);

	//Descarto la primera linea si es el hashbang!
	if (line[0] == '#' && line[1] == '!') {
		log_info(bgLogger, "Se leyó:%s", line);
		log_info(bgLogger,
				"No se pasa la linea anterior al nucleo por ser un hashbang.");
		length = 0;
		line = NULL;
		read = getline(&line, &length, programa);
	}

	while (read != -1) {
		log_info(bgLogger, "Se leyó:%s", line);
		string_append(&contenido, line);
		size += strcspn(line, "\n") + 1;
		free(line);
		length = 0; //no se si es necesario... pero nunca sobra xD
		read = getline(&line, &length, programa);
	}
	//free(line);
	string_append(&contenido, "\n\0");
	size += 2;
	log_info(bgLogger, contenido);
	log_debug(bgLogger, "Fin de archivo alcanzado. Tamaño almacenado: %d",
			size);

	send_w(cliente, headerToMSG(HeaderScript), 1);
	send_w(cliente, intToChar(size), 1); //fixme: un char admite de 0 a 255. SI el tamaño supera eso se rompe!
	send_w(cliente, contenido, size);

	free(contenido);
	log_debug(bgLogger, "Archivo enviado");
	fclose(programa); //Lo cierro aca asi se puede volver a ejecutar el mismo programa por mas que la consola este activa.
}

int main(int argc, char* argv[]) {
	system("clear");
	if (DEBUG_LOG_OLD_REMOVE) {
		log_warning(activeLogger, "DEBUG_LOG_OLD_REMOVE esta en true!");
		log_debug(activeLogger, "Borrando logs antiguos...");
		system("rm -rfv *.log");
	}
	//TODO lo de pasar los handshakes como un byte por meter el numero en un caracter, va barbaro! total son 5
	// ahora... con los headers que hacemos? cuando lleguemos al header 10 explota todo.
	crearLogs(string_from_format("consola_%d", getpid()), "Consola");
	char* path = malloc(PATHSIZE);
	log_info(activeLogger, "Soy consola de process ID %d.", getpid());

	if (DEBUG) {
		warnDebug();
	} else {
		if (argc == 1) {
			printf("Ingresar archivo ansisop: ");
			log_debug(bgLogger, "Se silicito ingresar un archivo ansisop.");
			scanf("%s", path);
		} else if (argc == 2) {
			path = argv[1];
		} else {
			log_error(activeLogger, "Muchos parametros.");
			log_info(activeLogger,
					"No poner parametros o poner solo el nombre del archivo a abrir");
			exit(EXIT_FAILURE);
		}
	}

	if (DEBUG) {
		log_debug(activeLogger, "Se va a abrir:%s", "facil.ansisop");
		programa = fopen("facil.ansisop", "r");
	} else {
		programa = fopen(path, "r");
	}

	if (programa == NULL) {
		perror("No se encontro el archivo.");
	}

	// Me conecto al nucleo y hago el handshake
	realizarConexion();

	// Paso el archivo a nucleo.
	cargarYEnviarArchivo();
	// Escucho pedidos (de impresion) hasta que el header que llegue sea de finalizar.
	// En ese caso se finaliza.
	escucharPedidos();
	// La finalizacion se hace por header de finalizacion de consola.
	return EXIT_SUCCESS;
}

