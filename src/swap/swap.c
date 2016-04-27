/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */


#include "../otros/handshake.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/header.h"
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/process.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>



#define PUERTOUMC 8081

/* la estructura que de cada proceso de procese(? */
typedef struct infoProcesos {
	int pid;
	int numPagina;
	int posPagina;
} t_infoProcesos;

typedef struct disponibles {
	int marcoInicial;
	int totalMarcos;
} t_disponibles;

t_config* archSwap; //archivo de configuracion
t_log* logSwap; //archivo de logs
t_list* espacioUtilizado; //lista de paginas usadas
t_list* espacioDisponible; //lista de paginas no usadas

// FUNCIONES UTILES
 // SOCKETS
	struct sockaddr_in direccionServidor;
    struct sockaddr_in direccionCliente;
    int servidor, cliente;
    unsigned int tamanioDireccion;



int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,sizeof(handshake_t));
	return &handshake;
}

void handshakear()
{
	if(getHandshake() != SOYUMC)
	{
		log_error(activeLogger,"Se conecto al nucleo algo que no es umc");
	}
	log_debug(bgLogger,"Swap recibio handshake");
	handshake_t hand = SOYUMC;
	send_w(cliente, (void*)&hand, sizeof(hand));
	log_debug(bgLogger,"Swap handshakeo");
}


void socketear()
{
	direccionServidor = crearDireccionParaServidor(PUERTOUMC);
	servidor = socket_w();
	int activado = 1;
	bind_ws(servidor,&direccionServidor);
	permitirReutilizacion(servidor,&activado);
	listen_w(servidor);
	log_debug(bgLogger,"Estoy escuchando");

	cliente = accept(servidor, (void*)&direccionCliente,&tamanioDireccion);
	log_debug(bgLogger,"Conexion aceptada");
	handshakear();
}






void manejoSwap()
{
	/*asignemos el archivo de configuracion "vamo' a asignarlo"*/
		archSwap = config_create("archivoConfigSwap");
		/*vamo' a leerlo*/
		char* puertoEscucha = config_get_string_value(archSwap, "PUERTO_ESCUCHA");
		char* nomSwap = config_get_string_value(archSwap, "NOMBRE_SWAP");
		int cantPaginasSwap = config_get_int_value(archSwap, "CANTIDAD_PAGINAS");
		int tamPag = config_get_int_value(archSwap, "TAMANIO_PAGINA");
		int retCompactacion = config_get_int_value(archSwap,"RETARDO_COMPACTACION");

		/*logs*/
		logSwap = log_create("logSwap", "TP", 0, LOG_LEVEL_TRACE);

		/* listas manejo de paginas */
		espacioUtilizado = list_create();
		espacioDisponible = list_create();

		t_disponibles* disponibles = malloc(sizeof(t_disponibles));
		disponibles.marcoInicial = 0; //primer marco
		disponibles->totalMarcos = cantPaginasSwap; //todos los marcos que son la misma cantidad que paginas
		list_add(espacioDisponible, disponibles);
		
		void socketear();
}





int main()
{




	/* crear archivo de tamaño configurable en bytes que representa particion de swap
	 * quedarse a la espera de conexion del proceso umc (ver comando dd para creacion de archivos)
	 * el archi swap se llena con '/0' para inicializar particion
	 * el tamaño de paginas escritas en swap es configurable y tambien el nombre de archivo
	 * se utiliza esquema de asignacion contigua (cap 11)
	 * la particion inicial es un hueco del total de su tamaño medido en cantidad de paginas que puede alojar
	 * cuando llega programa se asigna tamaño necesario para guardarlo dejando libre el espacio restante. Lo mismo ocurre con programas en ejecucion
	 * (SI finaliza se marca como libre el espacio asignado)
	 * Para administrar el espacion utilizado se utiliza un bitmap para saber si esta disponible o noy se emplea estructura de control (PID, NUmero de pagina, posicion de pagina)
	 * Cuado se informa creacion de programa busca hueco : Si no es suficiente s rechaza proceso y se cancela inicializacion
	 *                                                     SI hay fragmentacion externa (hay espacio pero no se puede asignar x fragmentacion) se compacta la particion
	 * Ante pedido de lectura de pagina realizado por umc este modulo devuelve contenido de pagina
	 * ante pedidod de ecritura de pagina se sobreescrine el contenido
	 * cuando se informa finalizacion de un programa se debe borrar de la particion de swap, marcando paginas ocupadas como disponibles
	 *
	 */
	return 0;
}
