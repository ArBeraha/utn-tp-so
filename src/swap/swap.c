/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */


#include "../otros/handshake.h"
#include "../otros/sockets/cliente-servidor.h"
#include <stdlib.h>
#include <stdio.h>
#include <commons/string.h>
#include <commons/config.h>

/* la estructura que de cada proceso de procese(? */
typedef struct infoProcesos {
	int pid;
	int numPagina;
	int posPagina;
} t_infoProcesos;

/*asignemos el archivo de configuracion "vamo' a asignarlo"*/
archSwap = config_create("archivoConfigSwap");
/*vamo' a leerlo*/
char* puertoEscucha = config_get_string_value(configSwap, "PUERTO_ESCUCHA");
char* nomSwap = config_get_string_value(configSwap, "NOMBRE_SWAP");
int cantPaginasSwap = config_get_int_value(configSwap, "CANTIDAD_PAGINAS");
int tamPag = config_get_int_value(configSwap, "TAMANIO_PAGINA");
int retCompactacion = config_get_int_value(configSwap,"RETARDO_COMPACTACION");



int tamTot = cantPaginasSwap * tamPag; /* tamaño total en bytes de mi swap*/





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
