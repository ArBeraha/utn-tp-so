/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */



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
#include <stdlib.h>
#include <stdio.h>
#include "serializacion.h"


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

t_config* archSwap;

t_bitarray* espacio; //Bitmap de espacio utilizado, voy a tener una posicion por cada marco
char *bitarray;


t_list* espacioUtilizado;
int espacioDisponible;



int espacioLibre;
int espacioOcupado;


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


// *******************************************************FUNCIONES UTILES**********************************************************

/*****PROTOTIPOS******/
void asignarEspacioANuevoProceso(int, int);
void agregarProceso(int, int);
void leerPagina(int, int );
void escribirPagina(int, int , int );
void finalizarProceso(int);
void procesarHeader(int, char*);
int espaciosDisponibles (t_bitarray*);
int espaciosUtilizados (t_bitarray*);
void limpiarPosiciones (t_bitarray* , int, int );
void setearPosiciones (t_bitarray*, int, int );
int hayQueCompactar(int);
t_infoProceso* buscarProceso(int);
int buscarMarcoInicial(int);
void sacarElemento(int);
//void compactar();
void archivoDeConfiguracion();
void funcionamientoSwap();
void asignarEspacioANuevoProceso(int, int);
void agregarProceso(int, int);
void leerPagina(int, int);
void escribirPagina(int, int , int );
void finalizarProceso(int);
//void test();





void procesarHeader(int cliente, char* header)
{
	char* payload;
	int payload_size;
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));
	char* pedido = malloc(sizeof(t_datosPedido));
    t_datosPedido datosPedido;


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
    			send_w(cliente, intToChar(SOYSWAP), 1);
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
    	    deserializar_int(&(datosPedido.pid), pedido);
    	    deserializar_int(&(datosPedido.pagina), pedido);
    		asignarEspacioANuevoProceso(datosPedido.pid, datosPedido.pagina);
    		break;


    	case HeaderOperacionLectura:
    		log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
    		deserializar_int(&(datosPedido.pid), pedido);
    		deserializar_int(&(datosPedido.pagina), pedido);
    		leerPagina(datosPedido.pid, datosPedido.pagina);
    		break;

    	case HeaderOperacionEscritura:
		    log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
		    deserializar_int(&(datosPedido.pid), pedido);
		    deserializar_int(&(datosPedido.pagina), pedido);
		    deserializar_int(&(datosPedido.tamanio), pedido);
		    escribirPagina(datosPedido.pid, datosPedido.pagina, datosPedido.tamanio);

		    break;

    	case HeaderOperacionFinalizarProceso:
    		log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
    		deserializar_int(&(datosPedido.pid), pedido);
    		finalizarProceso(datosPedido.pid);
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

void limpiarPosiciones (t_bitarray* unEspacio, int posicionInicial, int tamanioProceso)
{
	int i=0;
	for(i=posicionInicial; i < tamanioProceso; i++) bitarray_clean_bit(unEspacio, i);
}

void setearPosiciones (t_bitarray* unEspacio, int posicionInicial, int tamanioProceso)
{
	int i=0;
	for(i=posicionInicial; i < tamanioProceso; i++) bitarray_set_bit(unEspacio, i);
}


//Comprueba si hay fragmentacion externa
int hayQueCompactar(int paginasAIniciar) {
	int cantidadDePaginas = bitarray_get_max_bit(espacio);
	int i=0;
	int flag = 1;
	int marcos;
	for (i = 0; i < cantidadDePaginas; i++) {
		if(bitarray_test_bit(espacio,i)==0) marcos++;
		if(bitarray_test_bit(espacio,i)) marcos=0;
		if (marcos>= paginasAIniciar) {
			flag = 0;
		}
	}
	return flag;
}



t_infoProceso* buscarProceso(int pid) {
	int coincideElPID(t_infoProceso* datoProceso) {
		return (datoProceso->pid == pid);
	}
	t_infoProceso* datoProceso = (t_infoProceso*) list_find(
			espacioUtilizado, (void*) coincideElPID);
	return datoProceso;
}

int buscarMarcoInicial(int pid) {
	t_infoProceso* datoProceso = buscarProceso(pid);
	return datoProceso->posPagina;
}

void sacarElemento(int pid) {
	int coincideElPID(t_infoProceso* datoProceso) {
		return (datoProceso->pid == pid);
	}
	list_remove_by_condition(espacioUtilizado,
			(void*) coincideElPID);
}


//Funcion que busca en la lista de utilizados el proceso con el marco igual o mas proximo (mayor) al numero que se le pasa
t_infoProceso* elemMIMenor (marcoAComparar) {
	int cantElementos = list_size(espacioUtilizado);
	t_infoProceso* elem1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	int i;
	for (i = 0; i < cantElementos; i++) {
		elem1 = list_get(espacioUtilizado, i);
		if (elem1->posPagina >= marcoAComparar) {
			break;
		}
	}
	int j;
	for (j = 0; j < cantElementos; j++) {
		t_infoProceso* elem2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
		elem2 = list_get(espacioUtilizado, j);
		if (elem2->posPagina >= marcoAComparar && elem1->posPagina > elem2->posPagina) {
			elem1 = elem2;
		}
	}
	return elem1;
}


//void compactar() {
//	log_info(activeLogger, "Compactación iniciada por fragmentación externa.");
//	t_infoProceso* primerElemento = elemMIMenor(0);
//	if (primerElemento->posPagina != 0)
//	{
//		modificarArchivo(primerElemento->posPagina,
//				primerElemento->cantidadDePaginas, 0);
//		sacarElemento(primerElemento->pid);
//		primerElemento->posPagina = 0;
//		list_add(espacioUtilizado, primerElemento);
//	}
//	int cantElementos = list_size(espacioUtilizado);
//	int i = 0;
//	while (i < cantElementos - 1)
//	{
//		i++;
//		int marcoInicialSig = primerElemento->posPagina + primerElemento->cantidadDePaginas;
//		t_infoProceso* sigElemento = elemMIMenor(marcoInicialSig);
//		if (marcoInicialSig != sigElemento->posPagina) {
//			modificarArchivo(sigElemento->posPagina,
//					sigElemento->cantidadDePaginas, marcoInicialSig);
//			sacarElemento(sigElemento->pid);
//			sigElemento->posPagina = marcoInicialSig;
//			list_add(espacioUtilizado, sigElemento);
//			primerElemento = sigElemento;
//		} else {
//			primerElemento = sigElemento;
//		}
//	}
//
//    int marcoInicialHueco = primerElemento->posPagina + primerElemento->cantidadDePaginas;
//
//
//
//
//
//
//
//	list_clean(espacioDisponibleParaProcesos);
//	t_datoHueco* hueco = (t_datoHueco*) malloc(sizeof(t_datoHueco));
//	hueco->marcoInicial = marcoInicialHueco;
//	hueco->marcosTotales = cantidadPaginas - marcoInicialHueco;
//	list_add(espacioDisponibleParaProcesos, hueco);
//	sleep(retardoCompactacion);
//	log_info(logSwap, "Compactación iniciada por fragmentación externa.");
//}
//

void archivoDeConfiguracion()
{

	archSwap = config_create("archivoConfigSwap");

	puertoEscucha = config_get_string_value(archSwap, "PUERTO_ESCUCHA");
	nomSwap = config_get_string_value(archSwap, "NOMBRE_SWAP");
	cantPaginasSwap = config_get_int_value(archSwap, "CANTIDAD_PAGINAS");
	tamanioPag = config_get_int_value(archSwap, "TAMANIO_PAGINA");
	retCompactacion = config_get_int_value(archSwap,"RETARDO_COMPACTACION");
	retAcceso = config_get_int_value(archSwap, "RETARDO_ACCESO");

	// lo voy a usar para comando dd que requiere strings para mandar por comando a consola
	cantPag = config_get_string_value(archSwap, "CANTIDAD_PAGINAS");
	tamPag = config_get_string_value(archSwap, "TAMANIO_PAGINA");
}



/************************************FUNCIONAMIENTO DE SWAP********************************************************************/
void funcionamientoSwap()
{

        archivoDeConfiguracion();







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
		espacio = bitarray_create(bitarray,cantPaginasSwap);
		espacioUtilizado = list_create();
		//marco libres todos las posiciones del array
        limpiarPosiciones (espacio,0,cantPaginasSwap);




	//SOCKETS

	    char *header;
	    crearLogs("Swap","Swap");

	    printf("1");
		configurarServidor(PUERTO_SWAP);
		printf("2");
		log_info(activeLogger,"Esperando conexiones");
		printf("3");
		procesarNuevasConexiones();
		cliente=clientes[0].socket;
		printf("4");

		while (1){
			header=recv_waitall_ws(cliente,1);
			procesarHeader(cliente,header); //Incluye deserializacion
        }

        //testSwap();


}






//************************************FUNCIONES PRINCIPALES DE SWAP*********************************************************


void asignarEspacioANuevoProceso(int pid, int paginasAIniciar){

	if (paginasAIniciar <= espacioDisponible) {
	//Me fijo si hay fragmentacion para asi ver si necesito compactar
	if (hayQueCompactar(paginasAIniciar)) {
	 //compactar();
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
	int totalMarcos=0;
	int marcoInicial=0;
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
			pid, marcoInicial , string_length(buffer), buffer);
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


void escribirPagina(int pid, int paginaAEscribir, int tamanio) {
	//Abro el archivo de Swap
	FILE *archivoSwap;
	archivoSwap = fopen(nombreArchivo, "r+");
	if (archivoSwap == NULL) {
		printf("Error al abrir el archivo para escribir\n");
	}
	char* texto = malloc(tamanioPag);
	recv(cliente, (void*) texto, tamanio, MSG_WAITALL);
	//Al buffer que me envian para escribir lo lleno de ceros hasta completar el tamaño de página
	int i;
	for (i = tamanio; i < tamanioPag; i++) {
		texto[i] = '\0';
	}
	//Me posiciono en la página que quiero escribir y escribo
	int marcoInicial = buscarMarcoInicial(pid);
	int marcoAEscribir = (marcoInicial + paginaAEscribir); //TODO: marcoAEscribir señala el final del marco
	fseek(archivoSwap, marcoAEscribir, SEEK_SET);
	printf("texto:%s\n", texto);
	int exitoAlEscribir = fwrite(texto, tamanioPag, 1, archivoSwap);

	fclose(archivoSwap);
	usleep(retAcceso);

    //Chequeo si escribió
	if (exitoAlEscribir == 1) {
	    printf("Pagina escrita exitosamente\n");
		send_w(cliente, headerToMSG(HeaderEscrituraCorrecta),1 );
		log_info(activeLogger, "El Programa %d - Pagina Inicial:%d Tamanio:%d Contenido:%s. Escritura realizada correctamente.",
					pid, marcoInicial, tamanio, texto);
	} else {
		printf("Error al escribir pagina\n");
		send_w(cliente, headerToMSG(HeaderEscrituraErronea), 1);
	}

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





//**************************************************MAIN SWAP*****************************************************************

int main(int argc, char** argv)
{
    funcionamientoSwap();

	return 0;
}
























/****************************************TEST***************************************************************/

void testSwapDeBitArray1()
{
 bitarray_clean_bit(espacio, 0);
 bitarray_set_bit(espacio, 1);
 bitarray_set_bit(espacio, 2);
 bitarray_clean_bit(espacio, 3);
 bitarray_clean_bit(espacio, 4);
 bitarray_set_bit(espacio, 5);
 bitarray_clean_bit(espacio, 6);
 bitarray_clean_bit(espacio, 7);
 bitarray_set_bit(espacio, 8);
 setearPosiciones (espacio,9,cantPaginasSwap);

 if(hayQueCompactar(3)) printf("Test de posibilidad de compactacion superado\n");
 else printf("Test de posibilidad de compactacion no fue superado\n");
}

void testSwapDeBitArray2()
{
bitarray_clean_bit(espacio, 0);
bitarray_set_bit(espacio, 1);
bitarray_clean_bit(espacio, 2);
bitarray_clean_bit(espacio, 3);
bitarray_clean_bit(espacio, 4);
bitarray_set_bit(espacio, 5);
bitarray_clean_bit(espacio, 6);
bitarray_clean_bit(espacio, 7);
bitarray_set_bit(espacio, 8);
 setearPosiciones (espacio,9,cantPaginasSwap);

 if(hayQueCompactar(3)) printf("Test de posibilidad de compactacion no fue superado\n");
 else printf("Test de posibilidad de compactacion fue superado\n");
}

//void testFinalizarProceso();
//{
//	t_infoProceso* unProceso;
//	unProceso.pid;
//}
