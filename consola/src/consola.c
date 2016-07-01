/*
 * consola.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "consola.h"

void cargarConfig(){
	t_config* configConsola;
	configConsola = config_create("consola.cfg");
	config.puertoNucleo = config_get_int_value(configConsola, "PUERTO_NUCLEO");
	config.ipNucleo = config_get_string_value(configConsola, "IP_NUCLEO");
//	config.DEBUG = config_get_int_value(configConsola, "DEBUG");
//	config.DEBUG_LOG_OLD_REMOVE = config_get_int_value(configConsola, "DEBUG_LOG_OLD_REMOVE");
	config.DEBUG_RAISE_LOG_LEVEL = config_get_int_value(configConsola, "MOSTRAR_LOGS_EN_PANTALLA");
	config.DEBUG = false;
	config.DEBUG_LOG_OLD_REMOVE = false;
}

void sacarSaltoDeLinea(char* texto) // TODO testear! Hice esta funcion desde el navegador xD
{
	//Lee y termina por \n y \0, entonces si hay un \n lo piso con \0, y si hay un \0 lo piso con \0 (lease, no hago nada xD)
	texto[strcspn(texto, "\r\n")] = '\0'; //Anda para cualquier tipo de salto de linea, aunque no sea un \n.
}

void imprimirVariable() {
	char* msgValue = recv_waitall_ws(cliente, sizeof(ansisop_var_t));
	int value = char4ToInt(msgValue);
	// uso printf y logger de background solo porque es un mensaje impreso normalmente
	// y no algo del log.
	printf("Consola> %d\n", value);
	log_debug(debugLogger, "Mensaje impreso: |Consola> %d|", value);
	free(msgValue);
}

void imprimirTexto() {
	char* texto = leerLargoYMensaje(cliente);
	sacarSaltoDeLinea(texto);
	printf("Consola> %s\n", texto);
	// uso printf y logger de background solo porque es un mensaje impreso normalmente
	// y no algo del log.
	log_debug(debugLogger, "Mensaje impreso: |Consola> %s|", texto);
	free(texto);
}

void conectarANucleo() {
	direccion = crearDireccionParaCliente(config.puertoNucleo, config.ipNucleo);
	cliente = socket_w();
	connect_w(cliente, &direccion);
}

void finalizar() {
	log_info(activeLogger, "Fin exitoso del mulo consola.");
	destruirLogs();
	// el fclose se hace apenas se deja de usar el archivo para poder correr 2 instancias del mismo proceso ansisop.
	close(cliente);
	exit(EXIT_SUCCESS); //Un return desde main hace un exit, asi que es lo mismo hacerlo aca como exit!
}

void procesarHeader(char *header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	//log_debug(debugLogger, "Llego un mensaje con header %d.", charToInt(header));

	switch (charToInt(header)) {

	case HeaderError:
		log_error(errorLogger, "Header de Error.");
		break;

	case HeaderHandshake:
		log_error(errorLogger,
				"Segunda vez que se recibe un headerHandshake acá.");
		exit(EXIT_FAILURE);
		break;

	case HeaderImprimirVariableConsola:
		imprimirVariable();
		break;

	case HeaderImprimirTextoConsola:
		imprimirTexto();
		break;

	case HeaderConsolaFinalizarRechazado:
		log_info(activeLogger,"Proceso ansisop rechazado.");
		finalizar();
		break;

	case HeaderConsolaFinalizarNormalmente:
		finalizar();
		break;

	case HeaderConsolaFinalizarMuerteCPU:
		log_info(activeLogger,"Proceso terminado por desconexión de CPU.");
		destruirLogs();
		close(cliente);
		exit(EXIT_SUCCESS);
		break;

	case HeaderConsolaFinalizarErrorInstruccion:
	log_info(activeLogger,"Proceso terminado instrucción o parametro invalido");
	destruirLogs();
	close(cliente);
	exit(EXIT_SUCCESS);

	default:
		log_error(errorLogger, "Llego cualquier cosa.");
		log_error(errorLogger,
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

	log_debug(debugLogger, "Consola handshakeo.");
	if (getHandshake() != SOYNUCLEO) {
		perror("Se esperaba que la consola se conecte con el nucleo.");
	} else
		log_debug(debugLogger, "Consola recibio handshake de Nucleo.");
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
	log_info(activeLogger, "Conexion al nucleo correcta.");
	handshakear();
	log_info(activeLogger, "Handshake finalizado exitosamente.");
	log_debug(debugLogger, "Esperando algo para imprimir en pantalla.");
}

void warnDebug() {
	log_warning(warningLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
	log_info(activeLogger,
			"Para ingresar manualmente un archivo: Cambiar la configuracion de consola.");
	log_warning(warningLogger, "--- CORRIENDO EN MODO DEBUG!!! ---", getpid());
}

void cargarYEnviarArchivo() {
	log_debug(debugLogger, "Empezando la lectura del script.");
	size_t length = 0;
	int size = 0;
	char* line = NULL;
	char* contenido = string_new();
	ssize_t read = getline(&line, &length, programa);

	//Descarto la primera linea si es el hashbang!
//	if (line[0] == '#' && line[1] == '!') {
//		log_info(bgLogger, "Se leyó: |%s|", line);
//		log_info(bgLogger,
//				"No se pasa la linea anterior al nucleo por ser un hashbang.");
//		length = 0;
//		line = NULL;
//		read = getline(&line, &length, programa);
//	}

	while (read != -1) {
		log_info(bgLogger, "Se leyó: |%s|", line);
		string_append(&contenido, line);
		size += strcspn(line, "\n") + 1;
		free(line);
		length = 0; //no se si es necesario... pero nunca sobra xD
		read = getline(&line, &length, programa);
	}
	free(line);
	string_append(&contenido, "\0");
	size += 1;
	log_info(bgLogger, contenido);
	log_debug(debugLogger, "Fin de archivo alcanzado. Tamaño almacenado: |%d|",
			size);

	send_w(cliente, headerToMSG(HeaderScript), 1);
	send_w(cliente, intToChar4(size), sizeof(int));
	send_w(cliente, contenido, size);

	free(contenido);
	log_debug(debugLogger, "Archivo enviado");
	fclose(programa); //Lo cierro aca asi se puede volver a ejecutar el mismo programa por mas que la consola este activa.
}

int main(int argc, char* argv[]) {
	cargarConfig();

	crearLogs(string_from_format("consola_%d", getpid()), "Consola", config.DEBUG_RAISE_LOG_LEVEL);

	if (config.DEBUG) {
		warnDebug();
	} else {
		if (argc == 1) {
			printf("Ingresar archivo ansisop: ");
			log_debug(debugLogger, "Se silicito ingresar un archivo ansisop.");
			scanf("%s", path);
		} else if (argc == 2) {
			path = argv[1];
		} else {
			log_error(errorLogger, "Muchos argumentos.");
			log_info(activeLogger,
					"No poner argumentos o poner solo el nombre del archivo a abrir");
			exit(EXIT_FAILURE);
		}
	}

	if (config.DEBUG) {
		log_debug(debugLogger, "Se va a abrir: %s", "facil.ansisop");
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
	if(!config.DEBUG){
		cargarYEnviarArchivo();
	}

	// Escucho pedidos (de impresion) hasta que el header que llegue sea de finalizar.
	// En ese caso se finaliza.
	escucharPedidos();
	// La finalizacion se hace por header de finalizacion de consola.
	return EXIT_SUCCESS;
}

