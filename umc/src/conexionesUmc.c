/*
 * conexionesUmc.c
 *
 *  Created on: 19/6/2016
 *      Author: utnso
 */

#include "umc.h"

// HILO HIJO: cpu()
// FUNCION: Atender los headers de las cpus

void enviarASwap(int pid, tablaPagina_t* pagina){

	char* serialPID = intToChar4(pid);
	char* serialPagina = intToChar4(pagina->nroPagina);;
	char* contenidoPagina = malloc(config.tamanio_marco);
	memcpy(contenidoPagina,memoria+(pagina->marcoUtilizado * config.tamanio_marco),config.tamanio_marco);

	enviarHeader(swapServer,HeaderOperacionEscritura);
	send_w(swapServer,serialPID,sizeof(int));
	send_w(swapServer,serialPagina,sizeof(int));
	send_w(swapServer,contenidoPagina,config.tamanio_marco);
}

HILO hiloDedicado(int indice) {
	log_info(activeLogger, "Se creó un hilo dedicado");
	t_cliente clienteLocal; // Generamos una copia del cliente, no sirve para datos actualizables como el pid, solo para permanentes como socket e indice, para los demas campos consultar con mutex el vector de clientes
	MUTEXCLIENTES(clienteLocal = clientes[indice]);
	printf("SOCKET:%d\n",clienteLocal.socket);
	printf("INDICE:%d\n",clienteLocal.indice);
	char* header = malloc(1);
	while (recv(clienteLocal.socket, header, 1, MSG_WAITALL)>=0){
		procesarHeader(clienteLocal, header);
	}
	free(header);
	int pid;
	MUTEXCLIENTES(pid = clientes[indice].pid);
	log_info(activeLogger, "[%d]Hasta aqui llego el hilo",pid);
	return 0;
}

void atenderHandshake(t_cliente cliente){
	log_info(activeLogger, "Llego un handshake");
	char* handshake = malloc(1);
	read(cliente.socket,handshake,1);

	if ((charToInt(handshake) == SOYCPU) || (charToInt(handshake) == SOYNUCLEO)) {
		log_info(activeLogger,
				"Es un cliente apropiado! Respondiendo handshake");
		send(cliente.socket, intToChar(SOYUMC), 1, 0);
		if (charToInt(handshake) == SOYNUCLEO){
			// Acciones especificas de nucleo despues del handshake
			log_info(activeLogger,
					"Enviando tamaño de pagina a Nucleo");
			char* serialTamanio = intToChar4(config.tamanio_marco);
			send_w(cliente.socket,serialTamanio,sizeof(int));
			free(serialTamanio);
			char* tamanioStack = malloc(sizeof(int));
			read(cliente.socket , tamanioStack, sizeof(int));
			paginas_stack = char4ToInt(tamanioStack);
			log_info(activeLogger, "Se recibio el tamanio del Stack: %d",paginas_stack);
		}
		else if (charToInt(handshake) == SOYCPU){
			// Acciones especificas de cpu despues del handshake
//			MUTEXSWAP(ejemploSWAP(cliente));
//			MUTEXCLIENTES(quitarCliente(cliente.indice))
		}

	} else {
		log_error(activeLogger,
				"No es un cliente apropiado! rechazada la conexion");
		log_warning(activeLogger,"Se quitará al cliente %d.",cliente.indice);
		MUTEXCLIENTES(quitarCliente(cliente.indice))
	}
	free(handshake);
}

void servidorCPUyNucleoExtendido(){

}

int getHandshake()
{
	char* handshake = recv_nowait_ws(swapServer,1);
	return charToInt(handshake);
}

// FIN 5


// 6. Conexion a Swap
void handshakearASwap(){
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYUMC);
	send_w(swapServer, hand, 2);

	log_debug(bgLogger,"Umc handshakeo.");
	if(getHandshake()!=SOYSWAP)
	{
		perror("Se esperaba que la umc se conecte con el swap.");
	}
	else
		log_info(activeLogger,"Umc recibio handshake de Swap.");
}

void conectarASwap(){
	direccion = crearDireccionParaCliente(config.puerto_swap,config.ip_swap);  //CAMBIAR ESTO DE IP
	swapServer = socket_w();
	connect_w(swapServer, &direccion);
	log_info(activeLogger,"Conexion a swap correcta :).");
	handshakearASwap();
	log_info(activeLogger,"Handshake finalizado exitosamente.");
}



//void ejemploSWAP(t_cliente cliente){
//	char* serialPID = intToChar4(cliente.indice);
//		char* serialCantidadPaginas = intToChar4(5);
//		char* serialPagina = intToChar4(2);
//		char* contenidoPagina = malloc(config.tamanio_marco);
//		memcpy(contenidoPagina,"abcdefg",7);
//		bzero(contenidoPagina+7,config.tamanio_marco-7);
//
//		// INICIAR PROCESO
//		enviarHeader(swapServer,HeaderOperacionIniciarProceso);
//		send_w(swapServer,serialPID,sizeof(int));
//		send_w(swapServer,serialCantidadPaginas,sizeof(int));
//		char* header = recv_waitall_ws(swapServer,1);
//		if (charToInt(header)==HeaderProcesoAgregado)
//			printf("Contesto el proceso Agregado\n");
//		else
//		if (charToInt(header)==HeaderNoHayEspacio){
//			printf("No hay espacio\n");
//			return;}
//		else{
//			printf("Llego mierda %d\n",(int)header);return;}
//
//		// ESCRIBIR PAGINA
//		enviarHeader(swapServer,HeaderOperacionEscritura);
//		send_w(swapServer,serialPID,sizeof(int));
//		send_w(swapServer,serialPagina,sizeof(int));
//		send_w(swapServer,contenidoPagina,config.tamanio_marco);
//		header = recv_waitall_ws(swapServer,1);
//			if (charToInt(header)==HeaderEscrituraCorrecta)
//				log_info(activeLogger,"Escritura correcta");
//			else if (charToInt(header)==HeaderEscrituraErronea)
//				log_warning(activeLogger,"Escritura erronea");
//			else log_error(activeLogger,"Llego mierda al escribir");
//
//
//		// LEER PAGINA
//		enviarHeader(swapServer,HeaderOperacionLectura);
//		char* contenidoPagina2 = malloc(config.tamanio_marco+1);
//		send_w(swapServer,serialPID,sizeof(int));
//		send_w(swapServer,serialPagina,sizeof(int));
//
//		header = recv_waitall_ws(swapServer,1);
//		if (charToInt(header)==HeaderOperacionLectura)
//			log_info(activeLogger,"Contesto con la pagina");
//		else if (charToInt(header)==HeaderProcesoNoEncontrado)
//			log_warning(activeLogger,"No la encontró");
//		else log_error(activeLogger,"Llego mierda al leer");
//
//		contenidoPagina2 = recv_waitall_ws(swapServer,config.tamanio_marco);
//		contenidoPagina2[config.tamanio_marco]='\0';
//		log_info(activeLogger,"Llego el msg:%s",contenidoPagina2);
//	//	log_info(activeLogger,"Llego el contenido y es igual:%d\n",strcmp(contenidoPagina,contenidoPagina2)==0);
//
//		// FINALIZAR PROCESO
//		enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
//		send_w(swapServer,serialPID,sizeof(int));
//
//		header = recv_waitall_ws(swapServer,1);
//		if (charToInt(header)==HeaderProcesoEliminado)
//			log_info(activeLogger,"Se elimino bien");
//		else if (charToInt(header)==HeaderProcesoNoEncontrado)
//			log_warning(activeLogger,"Se elimino mal");
//		else log_error(activeLogger,"Llego mierda al leer");
//
//		free(serialPID);
//		free(header);
//		free(serialCantidadPaginas);
//		free(serialPagina);
//		free(contenidoPagina);
//		free(contenidoPagina2);
//}
