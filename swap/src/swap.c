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
#include "handshake.h"
#include "header.h"
#include "cliente-servidor.h"
#include "log.h"
#include "commonTypes.h"

#define PUERTO_SWAP 8082
int cliente;

/* la estructura que de cada proceso de procese(? */
typedef struct infoProcesos {
	int pid;
	int numInicialMarco;
	int cantMarcos;
} t_infoProcesos;


typedef struct disponibles {
	int marcoInicial;
	int totalMarcos;
} t_disponibles;

struct t_config* archSwap; //archivo de configuracion

char* nomArchivo; //nombre del archivo vacio que creare mediante dd
char* ddComand; // comando a mandar a consola para crear archivo mediante dd


struct t_list* espacioUtilizado; //lista de paginas usadas
struct t_list* espacioDisponible; //lista de paginas no usadas



// FUNCIONES UTILES
void procesarHeader(int cliente, char* header)
{
	char* payload;
	int payload_size;
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));

    	switch(charToInt(header))
    	{

    	case HeaderError:
    		log_error(activeLogger,"Header de Error.");
    		break;

    	case HeaderHandshake:
    		log_debug(bgLogger, "Llego un handshake");
    		payload_size = 1;;
    		payload = malloc(payload_size);
    		//read(socketCliente[cliente], payload, payload_size);
    		payload= recv_waitall_ws(cliente,payload_size);
    		log_debug(bgLogger, "Llego un mensaje con payload %d",
    				charToInt(payload));
    		if (charToInt(payload) == SOYUMC) {
    			log_debug(bgLogger,
    					"Es un cliente apropiado! Respondiendo handshake");
    			send(cliente, intToChar(SOYSWAP), 1, 0);
    		} else {
    			log_error(activeLogger,
    					"No es un cliente apropiado! rechazada la conexion");
    			log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
    			quitarCliente(cliente);
    		}
    		free(payload);
    		break;


    	default:
    		log_error(activeLogger,"Llego cualquier cosa.");
    		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
    		exit(EXIT_FAILURE);
    		break;
    	}
    }






//Comprueba si hay fragmentacion externa
int hayFragmentacionExterna(int paginasAIniciar) {
	int cantidadHuecos = list_size(espacioDisponible);
	int i;
	int flag = 1;
	for (i = 0; i < cantidadHuecos; i++) {
		t_disponibles* hueco = (t_disponibles*) list_get(
				espacioDisponible, i);
		if (hueco->totalMarcos >= paginasAIniciar) {
			flag = 0;
		}
	}
	return flag;
}




/********************************************************************************************************************/
void funcionamientoSwap()
{

	/*asignemos el archivo de configuracion "vamo' a asignarlo"*/
		archSwap = config_create("archivoConfigSwap");

		char* puertoEscucha = config_get_string_value(archSwap, "PUERTO_ESCUCHA");
		char* nomSwap = config_get_string_value(archSwap, "NOMBRE_SWAP");
		int cantPaginasSwap = config_get_int_value(archSwap, "CANTIDAD_PAGINAS");
		int tamanioPag = config_get_int_value(archSwap, "TAMANIO_PAGINA");
		int retCompactacion = config_get_int_value(archSwap,"RETARDO_COMPACTACION");

		// lo voy a usar para comando dd que requiere strings para mandar por comando a consola
		char* cantPag = config_get_string_value(archSwap, "CANTIDAD_PAGINAS");
		char* tamPag = config_get_string_value(archSwap, "TAMANIO_PAGINA");

		/*logs*/
		crearLogs("Swap","Swap");




		// dd if=/dev/zero of=archivoConfigSwap bs=tamPag count=cantPag
		espacioDisponible = cantPaginasSwap; //Para manejar la asignacion de paginas a procesos
		ddComand = string_new(); //comando va a contener a dd que voy a mandar a consola para que cree el archivo
		nomArchivo = config_get_string_value(archSwap, "NOMBRE_SWAP");
		string_append(&ddComand, "dd if=/dev/zero of="); //crea archivo input vacio
		string_append(&ddComand, nomArchivo); //con el nombre de la swap
		string_append(&ddComand, " bs="); //defino tamaño del archivo (de la memoria swap)
		string_append(&ddComand, tamPag);
		string_append(&ddComand, " count=");
		string_append(&ddComand, cantPag); //cuyo tamaño va a ser igual al tamaño de las paginas*cantidad de paginas
		printf("%s\n", ddComand);
		system(ddComand); //ejecuto comando



		/* listas manejo de paginas */
		espacioUtilizado = list_create();
		espacioDisponible = list_create();

		t_disponibles* disponibles = malloc(sizeof(t_disponibles));
		disponibles->marcoInicial = 0; //primer marco
		disponibles->totalMarcos = cantPaginasSwap; //todos los marcos que son la misma cantidad que paginas
		list_add(espacioDisponible, disponibles);



	//SOCKETS

	    char *header;
	    crearLogs("Swap","Swap");

	    //printf("1");
		configurarServidor(PUERTO_SWAP);
		//printf("2");
		log_info(activeLogger,"Esperando conexiones");
		//printf("3");
		procesarNuevasConexiones();
		cliente=clientes[0].socket;
		//printf("4");

		while (1){
			header=recv_waitall_ws(cliente,1);
			procesarHeader(cliente,header);
        }






		 //FUNCIONES PRINCIPALES DE SWAP


void asignarEspacioANuevoProceso(int pid, int paginasAIniciar){

	if (paginasAIniciar <= espacioDisponible) {
	//Me fijo si hay fragmentacion para asi ver si necesito compactar
	if (hayFragmentacionExterna(paginasAIniciar)) {
	 compactar();
	}
	//Agrego el proceso a la lista de espacio utilizado y actualizo la de espacio disponible. Ademas informa
	//de que la operacion fue exitosa
	agregarProceso(pid, paginasAIniciar);

	} else {
		//send_w(cliente, headerToMSG(HeaderNoHayEspacio), 1);
		printf("No hay espacio suficiente para asignar al nuevo proceso.\n");
		log_error(activeLogger, "Fallo iniciacion del programa %d ", pid);

				}
			}
void agregarProceso(int pid, int paginasAIniciar) {

	//Calculo la cantidad de elementos que tiene la lista de espacio disponible
	int cantidadHuecos = list_size(espacioDisponible);
	//Recorro la lista de espacio disponible hasta que encuentro un elemento que tenga la cantidad de marcas necesarios
	int i;
	for (i = 0; i < cantidadHuecos; i++) {
		t_disponibles* hueco = (t_disponibles*) list_get(
				espacioDisponible, i);
		if (hueco->totalMarcos >= paginasAIniciar) {
			//Asigno el marco inicial de espacio disponible como marco inicial del proceso
			// y modifico los valores del elemento de espacio disponible
			int marcoInicial = hueco->marcoInicial;
			hueco->totalMarcos -= paginasAIniciar;
			hueco->marcoInicial += paginasAIniciar;
			list_replace(espacioDisponible, i, (void*) hueco);

			//Si el espacio disponible quedo sin marcos se elimina de la lista
			if (hueco->totalMarcos == 0) {
				list_remove(espacioDisponible, i);
			}
			//Definimos la estructura del nuevo proceso con los datos correspondientes y lo agregamos a la lista de espacio utilizado
			t_infoProcesos* proceso = (t_infoProcesos*) malloc(
					sizeof(t_infoProcesos));
			proceso->pid = pid;
			proceso->numInicialMarco = marcoInicial;
			proceso->cantMarcos = paginasAIniciar;
			int fueAgregado = list_add(espacioUtilizado,
					(void*) proceso);
			char * respuesta = malloc(1);
			if (fueAgregado == -1) {
				printf("Error al iniciar el proceso\n");
				respuesta[0] = 'M';
				//send_w(cliente, headerToMSG(HeaderErrorParaIniciar), 1);
				return;
			} else {
				printf("Proceso agregado exitosamente\n");
				//Actualizo el espacio disponible
				espacioDisponible -= paginasAIniciar;
				log_info(activeLogger,
						"El programa %d - Byte Inicial:%d Tamanio:%d Iniciado correctamente.",
						pid, proceso->numInicialMarco * tamanioPag,
						proceso->cantMarcos * tamanioPag);
				//send_w(cliente, headerToMSG(HeaderProcesoAgregado), 1);
				return;

			}
		}
	}

	printf("Hay que compactar\n");

	//send_w(cliente, headerToMSG(HeaderHayQueCompactar), 1);

	return;
}

/*
		}

		void leerPagina(){
			mando contenido de pagina por sockets?
		}

		void escribirPagina(){
		  si ya esta escrita sobreescribir();
		}

		void finalizarProceso(){
		 borrar de la particion de swap
		 marcar paginas ocupadas como disponibles
		}

        */

}


int main()
{
    funcionamientoSwap();

	return 0;
}
