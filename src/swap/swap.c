/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */


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
#include "../otros/handshake.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/header.h"
#include "../otros/log.h"



#define PUERTO_SWAP 8082

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

struct t_config* archSwap; //archivo de configuracion

struct t_list* espacioUtilizado; //lista de paginas usadas
struct t_list* espacioDisponible; //lista de paginas no usadas

// FUNCIONES UTILES
 // SOCKETS
	struct sockaddr_in direccionServidor;
	struct sockaddr_in direccionCliente;
    int servidor, cliente, socketUmc;

void procesarHeader(int cliente, char *header)
{
    	// Segun el protocolo procesamos el header del mensaje recibido
    	log_debug(bgLogger,"Llego un mensaje con header %d.",charToInt(header));

    	switch(charToInt(header))
    	{

    	case HeaderError:
    		log_error(activeLogger,"Header de Error.");
    		break;

    	case HeaderHandshake:
    		//log_error(activeLogger,"Segunda vez que se recibe un headerHandshake acá.");
    		//exit(EXIT_FAILURE);

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



void handshakearAUmc()
{
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYSWAP);
	send_w(cliente, hand, 2);

	//log_debug(bgLogger,"Consola handshakeo.");
	if(getHandshake()!=SOYUMC)
	{
		perror("Se esperaba que la swap se conecte con la umc.");
	}
	else
		log_debug(bgLogger,"Swap recibio handshake de umc.");

}

void realizarConexionAUmc()
{
	//char header[1];
    printf("1");
	configurarServidor(PUERTO_SWAP);
	printf("2");
	log_info(activeLogger,"Esperando conexiones");
	printf("3");
	procesarNuevasConexiones();
	printf("4");
	handshakearAUmc();
	printf("5");
	log_info(activeLogger,"Handshake finalizado exitosamente.");
	printf("6");
//
//	FD_ZERO(&socketsParaLectura);
//    FD_SET(socketUmc, &socketsParaLectura);
//     if (tieneLectura(socketUmc))
//     procesarNuevasConexiones(&socketUmc);
//     if (read( socketUmc , header, 1) != 0)
//     {
//    					log_debug(bgLogger,"LLEGO main %s",header);
//    					procesarHeader(cliente,header);
//     }


}
void manejoSwap()
{
//	/*asignemos el archivo de configuracion "vamo' a asignarlo"*/
//		archSwap = config_create("archivoConfigSwap");
//		/*vamo' a leerlo*/
//		char* puertoEscucha = config_get_string_value(archSwap, "PUERTO_ESCUCHA");
//		char* nomSwap = config_get_string_value(archSwap, "NOMBRE_SWAP");
//		int cantPaginasSwap = config_get_int_value(archSwap, "CANTIDAD_PAGINAS");
//		int tamPag = config_get_int_value(archSwap, "TAMANIO_PAGINA");
//		int retCompactacion = config_get_int_value(archSwap,"RETARDO_COMPACTACION");
//
//		/*logs*/
		crearLogs("Swap","Swap");
//
//		/* listas manejo de paginas */
//		espacioUtilizado = list_create();
//		espacioDisponible = list_create();
//
//		t_disponibles* disponibles = malloc(sizeof(t_disponibles));
//		disponibles->marcoInicial = 0; //primer marco
//		disponibles->totalMarcos = cantPaginasSwap; //todos los marcos que son la misma cantidad que paginas
//		list_add(espacioDisponible, disponibles);
//
//	direccion = crearDireccionParaServidor(PUERTO_SWAP);
//	socketNuevasConexiones = socket_w();
//	permitirReutilizacion(socketNuevasConexiones,&activado);
//	bind_ws(socketNuevasConexiones,&direccion);
//	listen_w(socketNuevasConexiones);
//	tamanioDireccion=sizeof(direccion);
//
//	struct sockaddr_in clienteDir;
//	unsigned int clienteLen = sizeof(clienteDir);
//	int socketNuevoCliente;
//	socketNuevoCliente = accept(socketNuevasConexiones, (struct sockaddr *)&clienteDir, (socklen_t*)&clienteLen);
//	printf("Nueva conexión , socket %d , ip is : %s , puerto : %d \n" , socketNuevoCliente , inet_ntoa(clienteDir.sin_addr) , ntohs(clienteDir.sin_port));
//	agregarCliente(socketNuevoCliente);
//
//	handshakearAUmc();




}




int main()
{
    manejoSwap();
	realizarConexionAUmc();






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

