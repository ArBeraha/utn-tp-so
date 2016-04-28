/*
 * umc.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/log.h"
#include "../otros/commonTypes.h"

#define PUERTO_UMC_NUCLEO 8081
#define MARCO 10
#define MARCO_SIZE 100 //en bytes
#define DEBUG true
#define PUERTO_SWAP 8082
#define ENTRADAS_TLB 10
#define RETARDO 5 //En MICRO SEGUNDOS: 1 micro = 1000 milisegundos

//Prototipos
void procesarHeader(int cliente, char *header);
//Fin prototipos

typedef struct tlbStruct{
	int pid,
		pagina;
	char* direccion;
}tlb_t;

typedef struct pedidoAUmc{
	 int idPrograma,
		 paginasRequeridas,
		 nroPagina,
		 offset,
		 tamanio,
		 buffer;
}pedidoAUmc_t;

typedef struct tablaPag{ //No hace falta indicar el numero de la pagina, es la posicion
	int pid;
	int marcoUtilizado;
	char bitPresencia;
	char bitModificacion;
	char bitUso;
	int* punteroAMarco;
}tablaPagina_t;

typedef struct marco{
    int indice;
    int uso;
    void* contenido;
}marco_t;

int tamanioMemoria = MARCO * MARCO_SIZE;

typedef int ansisop_var_t;
int cliente;
t_log *activeLogger, *bgLogger;
char* memoria;
tlb_t tlb[ENTRADAS_TLB];
tlb_t* ptlb;
int retardo = RETARDO;

struct marco_t* marco = malloc(MARCO * sizeof(marco_t));
struct t_queue* marcosLibres;

struct timeval newEspera()
{
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}

int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,1);
	return charToInt(handshake);
}


//1. Funciones principales de UMC

void inicializarPrograma(int idPrograma, int paginasRequeridas){
}

/*
char* devolverBytesDeUnaPagina(int nroPagina,int offset, int tamanioALeerAPartirDeOffset){ //nroPag, desde donde, hasta donde. Pag entera es Pag1,0,MARCO_SIZE
	size_t tamanioALeer = tamanioALeerAPartirDeOffset;
	if(existeLaPaginaYEstaEnMemoria(nroPagina)){
		usleep(retardo);
		int marco = tablaPaginas[nroPagina].marcoUtilizado;
		int pos = (marco * MARCO_SIZE) + offset;
		char *infoBuscada;
		//memcpy(infoBuscada,&memoria[pos],tamanioALeer); // NO FUNCA: VER

		log_info(activeLogger,"Se devolvio la pagina %d, offset &d, con %d bytes", nroPagina,offset,tamanioALeer);
		return infoBuscada;   //CAMBIAR LOS CHAR* POR ANSISOP_T, POR EN REALIDAD SON INTS, NADA DE CHAR PAPA
	}
	else{
		log_error(activeLogger,"No existe la pagina numero %d.", getpid());
		return NULL;
	}
}
*/

void almacenarBytesEnUnaPagina(int nroPagina, int offset, int tamanio, int buffer){
}

void finalizarPrograma(int idPrograma){
}


//FIN 1


//2. Funciones que se mandan por consola

void fRetardo(){
	int nuevoRetardo;
	printf("Ingrese el nuevo valor de Retardo en milisegundos: ");
	scanf("%d",nuevoRetardo);
	retardo = nuevoRetardo*1000;
}
void dumpEstructuraMemoria(){
}
void dumpContenidoMemoria(){
}
void flushTlb(){
}
void flushMemory(){
}

void recibirComandos(){
	int funcion;
	do {
		printf("Funciones: 0.salir / 1.retardo / 2.dumpEstructuraMemoria / 3.dumpContenidoMemoria / 4.flushTlb / 5.flushMemory \n");
		printf("Funcion: ");
		scanf("%d ",&funcion);

		switch(funcion){
			case 1: fRetardo();
			case 2: dumpEstructuraMemoria();
			case 3: dumpContenidoMemoria();
			case 4: flushTlb();
			case 5: flushMemory();
			default: break;
		}
	}
	while(funcion!=0);
}
// FIN 2


// 3.Server de los cpu y de nucleo
void servidorCPUyNucleo(){

	int mayorDescriptor, i;
	struct timeval espera = newEspera(); 		// Periodo maximo de espera del select
	char header[1];

	crearLogs("Umc","Umc");
	configurarServidor(PUERTO_UMC_NUCLEO);
	inicializarClientes();
	log_info(activeLogger,"Esperando conexiones ...");

	while(1){
		mayorDescriptor = incorporarSockets();
		select( mayorDescriptor + 1 , &socketsParaLectura , NULL , NULL , &espera);

		if (tieneLectura(socketNuevasConexiones))
			procesarNuevasConexiones();

		for (i = 0; i < getMaxClients(); i++){
			if (tieneLectura(socketCliente[i]))	{
				if (read( socketCliente[i] , header, 1) == 0)
					quitarCliente(i);
				else{
					log_debug(bgLogger,"LLEGO main %c\n",header);
					procesarHeader(i,header);
				}
			}
		}
	}
	destruirLogs();
}
// FIN 3


// 4. Conexion a Swap
void handshakearASwap(){
	char *hand = string_from_format("%c%c",HeaderHandshake,SOYUMC);
	send_w(cliente, hand, 2);

	log_debug(bgLogger,"Umc handshakeo.");
	if(getHandshake()!=SOYSWAP)
	{
		perror("Se esperaba que la umc se conecte con el swap.");
	}
	else
		log_debug(bgLogger,"Umc recibio handshake de Swap.");
}

void conectarASwap(){
	direccion = crearDireccionParaCliente(PUERTO_SWAP);
	cliente = socket_w();
	connect_w(cliente, &direccion);

	handshakearASwap();
}

void realizarConexionASwap()
{
	conectarASwap();
	log_info(activeLogger,"Conexion a swap correcta :).");
	handshakearASwap();
	log_info(activeLogger,"Handshake finalizado exitosamente.");
	log_debug(bgLogger,"Esperando algo para imprimir en pantalla.");
}

void escucharPedidosDeSwap(){
	char* header;
	while(true){
			header = recv_waitall_ws(cliente,sizeof(char));
			procesarHeader(cliente,header);
			free(header);
	}
}
// FIN 4

void crearMemoriaYTlbYTablaPaginas(){

	//Creo memoria y la relleno
	memoria = malloc(tamanioMemoria);
	memset(memoria,'\0',tamanioMemoria);
	log_info(activeLogger,"Creada la memoria.");

	//Creo cola de marcos disponibles
	int j;
	struct marco_t marco;
	marco.pid=j;
	marco.usado=0;
	marco.contenido=NULL;
	for(j=0;j<MARCO;j++){
		queue_enqueue(marcosLibres,marco);
	}
	log_info(activeLogger,"Creada cola de marcos listos, con la cantidad configurada por archivo.");


	//Relleno TLB
	int i;
	for(i = 0; i<ENTRADAS_TLB; i++){
		tlb[i].pid=-1;
		tlb[i].pagina=-1;
		tlb[i].direccion=NULL;
	}
	log_info(activeLogger,"Creada la TLB y rellenada con ceros (0).");
}


void procesarHeader(int cliente, char *header){
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger,"Llego un mensaje con header %d\n",charToInt(header));

	switch(charToInt(header)) {

	case HeaderError:
		log_error(activeLogger,"Header de Error\n");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger,"Llego un handshake\n");
		payload_size=1;
		payload = malloc(payload_size);
		read(socketCliente[cliente] , payload, payload_size);
		log_debug(bgLogger,"Llego un mensaje con payload %d\n",charToInt(payload));
		if ( (charToInt(payload)==SOYCPU) || (charToInt(payload)==SOYNUCLEO) ){
			log_debug(bgLogger,"Es un cliente apropiado! Respondiendo handshake\n");
			send(socketCliente[cliente], intToChar(SOYUMC), 1, 0);
		}
		else {
			log_error(activeLogger,"No es un cliente apropiado! rechazada la conexion\n");
			log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
			quitarCliente(cliente);
		}
		free(payload);
		break;

	case HeaderReservarEspacio:
		//char* pedidoPagina = recv_waitall_ws(cliente, sizeof(int)); //ES NECESARIO TENER EL PID DEL PROCESO Q NUCLEO QUIERE GUARDAR EN MEMORIA? SI: RECIBIR INT  NO: RECIBIR NADA
		log_info(activeLogger,"Nucleo me pidio memoria");
		int cantPaginasPedidas;
		struct t_list* tablaPaginas;

		//if(queue_pop(marcosLibres)){
		//
		//}


		case HeaderPedirPagina:
			log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
			//char* pedidoPagina = recv_waitall_ws(cliente, sizeof(pedidoAUmc_t));
			//char* devolucion = devolverBytesDeUnaPagina(pedidoPagina); //*** VER QUE LE MANDO!! * pedidoPagina??
			//send(devolucion)

		case HeaderGrabarPagina:
			log_info(activeLogger,"Se recibio pedido de grabar una pagina, por CPU");

		case HeaderLiberarRecursosPagina:
			log_info(activeLogger,"Se recibio pedido de liberar una pagina, por CPU");

		default:
			log_error(activeLogger,"Llego cualquier cosa.");
			log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
			log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
			quitarCliente(cliente);
			break;
	}
}

int main(void) {

	crearLogs("Umc","Umc");
	log_info(activeLogger,"Soy umc de process ID %d.", getpid());

	crearMemoriaYTlb();


	realizarConexionASwap();
	escucharPedidosDeSwap();

	servidorCPUyNucleo(); //OJO! A cada cpu hay que atenderla con un hilo



	recibirComandos(); //Otro hilo?

	free(memoria);

	return 0;
}

/*Ari's tips
 Ro, yo lo veo así al asunto, las páginas no existen antes que te pidan algo,
 sólo vas a tener la tabla de marcos que es la memoria real,  la tabla de páginas
 para mi debería ser una lista, ya que para cada proceso vamos a saber en tiempo
 de ejecución cuantas páginas necesita. Una vez que te piden espacio creas una tabla
 de paginas(lista) y recorres el array de marcos y cada espacio que encontras
 (no necesariamente contiguo) agregas un nodo a la lista con presencia 1, una vez que
 terminaste el array de marcos y faltan páginas por ubicar llamas al swap para ver si
 existe espacio en el almacenamiento secundario (y el swap hace algo parecido y te responde),
 esos nodos van a tener presencia 0
finalmente respondes si pudiste o no ubicar la cantidad de páginas solicitadas
 */

/*
 Entonces, cada vez que me piden paginas creo una nueva lista de paginas para ese PID

 Los marcos sin usar los tengo en un cola, asi es mas rapdio, para agarrar uno: queue_pop(marcosLibres)

 struct marco{
     int indice;
     int uso;
     void* contenido;
}


 */
