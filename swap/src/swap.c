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
#include <commons/bitarray.h>
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

typedef struct infoProceso{
	int pid;
	int numPagina;
	int posPagina;
	int cantidadDePaginas;
} t_infoProceso;

typedef struct datoPedido{
int pid;
int pagina;
int tamanio;
}t_datosPedido;

t_bitarray* espacio; //Bitmap de espacio utilizado, voy a tener una posicion por cada marco
t_list* espacioUtilizado;
int espacioDisponible;


int espacioLibre;
int espacioOcupado;

struct t_config* archSwap; //archivo de configuracion

char* nomArchivo; //nombre del archivo vacio que creare mediante dd
char* ddComand; // comando a mandar a consola para crear archivo mediante dd

char* nombreArchivo;
char* puertoEscucha;
char* nomSwap;
int cantPaginasSwap;
int tamanioPag;
int retCompactacion;
int retAcceso;
char* cantPag;
char* tamPag;


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
    		read(cliente, payload, payload_size);
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

    	case HeaderOperacionIniciarProceso:
    	    log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
    		t_datosPedido pedido;
    		pedido.pid = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		pedido.pagina = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		pedido.tamanio = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		asignarEspacioANuevoProceso(pedido.pid, pedido.pagina);
    		break;


    	case HeaderOperacionLectura:
    		log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
    		t_datosPedido pedido;
    		pedido.pid = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		pedido.pagina = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		pedido.tamanio = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		leerPagina(pedido.pid, pedido.pagina);
    		break;

    	case HeaderOperacionEscritura:
		    log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
		    t_datosPedido pedido;
		    pedido.pid = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
		    pedido.pagina = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
		    pedido.tamanio = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
		    escribirPagina(pedido.pid, pedido.pagina, pedido.tamanio);
		    break;

    	case HeaderOperacionFinalizarProceso:
    		log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
    	    t_datosPedido pedido;
    	    pedido.pid = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		pedido.pagina = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		pedido.tamanio = deserializar_int(recv_waitall_ws(cliente, sizeof(int)));
    		finalizarProceso(pedido.pid);
    		break;



    	default:
    		log_error(activeLogger,"Llego cualquier cosa.");
    		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
    		exit(EXIT_FAILURE);
    		break;
    	}
    }

int espaciosDisponibles (t_bitarray* unEspacio)
{
  int i;
  int espacioSinUtilizar=0;
  for(i=0; i<(bitarray_get_max_bit(unEspacio)); i++)
  {
	  if (bitarray_test_bit(unEspacio, i)==0) espacioSinUtilizar++ ;
  }
  return espacioSinUtilizar;
}

int espaciosUtilizados (t_bitarray* unEspacio)
{
  int i;
  int espacioUtilizado=0;
  for(i=0; i<(bitarray_get_max_bit(unEspacio)); i++)
  {
	  if (bitarray_test_bit(unEspacio, i)==0) espacioUtilizado++ ;
  }
  return espacioUtilizado;
}





//Comprueba si hay fragmentacion externa
int hayFragmentacionExterna(int paginasAIniciar) {
	int cantidadMarcos = bitarray_get_max_bit(espacio);
	int i=0;
	int flag = 1;
	int marcos;
	for (i = 0; i < cantidadMarcos; i++) {
		if(bitarray_test_bit(espacio,i)==0) marcos++;

		if (marcos>= paginasAIniciar) {
			flag = 0;
		}
	}
	return flag;
}

int buscarMarcoInicial(int pid) {
	t_infoProceso* datoProceso = buscarProceso(pid);
	return datoProceso->posPagina;
}

t_infoProceso* buscarProceso(int pid) {
	int coincideElPID(t_infoProceso* datoProceso) {
		return (datoProceso->pid == pid);
	}
	t_infoProceso* datoProceso = (t_infoProceso*) list_find(
			espacioUtilizado, (void*) coincideElPID);
	return datoProceso;
}

void sacarElemento(int pid) {
	int coincideElPID(t_infoProceso* datoProceso) {
		return (datoProceso->pid == pid);
	}
	list_remove_by_condition(espacioUtilizado,
			(void*) coincideElPID);
}




/********************************************************************************************************************/
void funcionamientoSwap()
{

	/*asignemos el archivo de configuracion "vamo' a asignarlo"*/
		archSwap = config_create("archivoConfigSwap");

		puertoEscucha = config_get_string_value(archSwap, "PUERTO_ESCUCHA");
		nomSwap = config_get_string_value(archSwap, "NOMBRE_SWAP");
		cantPaginasSwap = config_get_int_value(archSwap, "CANTIDAD_PAGINAS");
		tamanioPag = config_get_int_value(archSwap, "TAMANIO_PAGINA");
		retCompactacion = config_get_int_value(archSwap,"RETARDO_COMPACTACION");
		retAcceso = config_get_int_value(archSwap, "RETARDO_ACCESO");

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
		string_append(&ddComand, tamPag); //Lo siguiente no va ya que ahora mi memoria se divide en paginas, no bytes
		//string_append(&ddComand, " count=");
		//string_append(&ddComand, cantPag); //cuyo tamaño va a ser igual al tamaño de las paginas*cantidad de paginas
		printf("%s\n", ddComand);
		system(ddComand); //ejecuto comando



		/* bitarray manejo de paginas */
		bitarray_create(espacio,cantPaginasSwap);
		espacioUtilizado = list_create();




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
			procesarHeader(cliente,header); //Incluye deserializacion
        }




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
		send_w(cliente, headerToMSG(HeaderNoHayEspacio), 1);
		printf("No hay espacio suficiente para asignar al nuevo proceso.\n");
		log_error(activeLogger, "Fallo la iniciacion del programa %d ", pid);

				}
			}
void agregarProceso(int pid, int paginasAIniciar) {

	//Recorro  espacio disponible hasta que encuentro un elemento que tenga la cantidad de marcas necesarios //REFACTOR
	int cantidadHuecos = bitarray_get_max_bit(espacio);
	int i;
	int j;
	int totalMarcos;
	int marcoInicial;
	//Recorro el bitarray hasta que encuentro un hueco ocupado
	for (i = 0; i < cantidadHuecos; i++)
	{
		//Recorro el bitarray hasta que encuentro un hueco ocupado
		if(bitarray_test_bit(espacio,i)==0) totalMarcos++;
        //Si ese hueco me permite alojar las paginas
		if (totalMarcos>= paginasAIniciar)
		{
			//alojo el proceso (marco como ocupado)
			for(j=0; j < totalMarcos; j++)
            bitarray_set_bit(espacio, j); //Creo que los pone en 1, porque el clean los debe poner en 0
			//Definimos la estructura del nuevo proceso con los datos correspondientes y lo agregamos al espacio utilizado
			t_infoProceso* proceso = (t_infoProceso*) malloc(sizeof(t_infoProceso));
			proceso->pid = pid;
			proceso->posPagina = marcoInicial;
			proceso->cantidadDePaginas = paginasAIniciar;
			int fueAgregado = list_add(espacioUtilizado,(void*) proceso);
			if (fueAgregado == -1) {
				printf("Hubo un error al iniciar el proceso\n");
				send_w(cliente, headerToMSG(HeaderErrorParaIniciar), 1);
				return;
			} else {
				printf("Proceso agregado exitosamente\n");
				//Actualizo el espacio disponible
				espacioDisponible -= paginasAIniciar;
				log_info(activeLogger,
						"El programa %d cuyo Marco Inicial es:%d y su Tamanio es:%d fue Iniciado correctamente.",
						pid, proceso->posPagina,
						proceso->cantidadDePaginas * tamanioPag);
				send_w(cliente, headerToMSG(HeaderProcesoAgregado), 1);
				return;

		     }

		}
		marcoInicial++;


	printf("Hay que compactar\n");

	send_w(cliente, headerToMSG(HeaderHayQueCompactar), 1);

	return;
  }
}



void leerPagina(int pid, int paginaALeer) {


char* buffer = malloc(tamanioPag + 1);

//Abro el archivo de Swap
	FILE *archivoSwap;
	archivoSwap = fopen(nomArchivo, "r");
	if (archivoSwap == NULL) {
		printf("Error al abrir el archivo para leer\n");
	}

//Me posiciono en la página que quiero leer y guardo lo que leo en el buffer
	int marcoInicial = buscarMarcoInicial(pid); //TODO
	int marcoALeer = (marcoInicial + paginaALeer);
	int exitoAlLeer = fseek(archivoSwap, marcoALeer, SEEK_SET);
	fread(buffer, tamanioPag, 1, archivoSwap);
	fclose(archivoSwap);
	usleep(retAcceso);
	//mirar si no se puede leer
	log_info(activeLogger, "El programa %d cuyo Marco Inicial es:%d de Tamanio:%d .Contenido:%s. Lectura realizada correctamente.",
			pid, marcoInicial * tamanioPag, string_length(buffer), buffer);
	if (exitoAlLeer == 0) // si se leyo bien la pagina
			{
		send_w(cliente, headerToMSG(HeaderLecturaCorrecta), 1); //TODO

		//Envio lo leído a memoria
		send_w(cliente, (void*) buffer, tamanioPag);
		buffer[tamanioPag] = '\0';
		printf("Lectura exitosa : %s\n", buffer);

	} else {
		send_w(cliente,headerToMSG(HeaderLecturaErronea), 1);  //TODO
		printf("Error al intentar leer\n");

	}

//Libero el malloc del buffer
	free(buffer);
}

		void escribirPagina(int pid, int paginaAEscribir, int tamanio) //TODO
		{

		}


void finalizarProceso(int pid) {
	//Busco el proceso en la lista de espacio utilizado, guardo sus datos y lo elimino de la lista
	t_infoProceso* proceso;
	proceso = buscarProceso(pid); //TODO QUE PASA SI NO LO ENCUENTRA??

    //Actualizo la variable espacioDisponible
	espacioDisponible += proceso->cantidadDePaginas;
	int i;
	for (i = proceso->posPagina; i < proceso->cantidadDePaginas; i++)
	{
		bitarray_clean_bit(espacio, i);
	}
	sacarElemento(pid);
    usleep(retAcceso);
	printf("Proceso eliminado exitosamente\n");
	log_info(activeLogger, "El programa %d - Pagina Inicial:%d Tamanio:%d Eliminado correctamente.",
	pid, proceso->posPagina, proceso->cantidadDePaginas);
	send_w(cliente, headerToMSG(HeaderProcesoEliminado), 1);
	free(proceso);
			}







int main(int argc, char** argv)
{
    funcionamientoSwap();

	return 0;
}
