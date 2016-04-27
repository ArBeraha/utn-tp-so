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
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "../otros/handshake.h"
#include "../otros/header.h"
#include "../otros/sockets/cliente-servidor.h"
#include "../otros/log.h"

#define PUERTO_UMC_NUCLEO 8081
#define MARCO 10
#define MARCO_SIZE 100 //en bytes
#define DEBUG true
#define PUERTO_SWAP 8082
#define ENTRADAS_TLB 10
#define RETARDO 5 //En MICRO SEGUNDOS: 1 micro = 1000 milisegundos

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
}tablaPagina_t;

int tamanioMemoria = MARCO * MARCO_SIZE;

typedef int ansisop_var_t;
int cliente;
t_log *activeLogger, *bgLogger;
char* memoria;
tlb_t tlb[ENTRADAS_TLB];
tlb_t* ptlb;
tablaPagina_t tablaPaginas[MARCO];
int retardo = RETARDO;

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

	case HeaderPedirPagina:
		log_info(activeLogger,"Se recibio pedido de pagina de Nucleo");

	case HeaderGrabarPagina:

	case HeaderLiberarRecursosPagina:



	case HeaderScript: /*A implementar*/ break; //TODO
	/* Agregar futuros casos */

	default:
		log_error(activeLogger,"Llego cualquier cosa.");
		log_error(activeLogger,"Llego el header numero %d y no hay una acción definida para él.",charToInt(header));
		log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
		quitarCliente(cliente);
		break;
	}
}


//1. Funciones principales de UMC

char existeLaPaginaYEstaEnMemoria(int nroPagina){
	tablaPaginas[nroPagina].bitPresencia==1 && nroPagina<=MARCO?1:0;
}

void inicializarPrograma(int idPrograma, int paginasRequeridas){
}

char* devolverBytesDeUnaPagina(int nroPagina,int offset, int tamanioALeer){ //nroPag, desde donde, hasta donde. Pag entera es Pag1,0,MARCO_SIZE
	if(existeLaPaginaYEstaEnMemoria(nroPagina)){
		usleep(retardo);
		int marco = tablaPaginas[nroPagina].marcoUtilizado;
		int pos = (marco * MARCO_SIZE) + offset;
		char *infoBuscada;
		memcpy(infoBuscada,&memoria[pos],tamanioALeer);

		log_info(activeLogger,"Se devolvio la pagina %d, offset &d, con %d bytes", nroPagina,offset,tamanioALeer());
		return infoBuscada;   //CAMBIAR LOS CHAR* POR ANSISOP_T, POR EN REALIDAD SON INTS, NADA DE CHAR PAPA
	}
	else{
		log_error(activeLogger,"No existe la pagina numero %d.", getpid());
		return NULL;
	}
}

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
	memoria = (char*)malloc(tamanioMemoria);
	log_info(activeLogger,"Creada la memoria.");

	//Relleno TLB
	int i;
	for(i = 0; i<ENTRADAS_TLB; i++){
		tlb[i].pid=-1;
		tlb[i].pagina=-1;
		tlb[i].direccion=NULL;
	}
	log_info(activeLogger,"Creada la TLB y rellenada con ceros (0).");

	//Creo puntero a tabla y relleno tabla paginas
	struct tablaPagina_t* pTablaPaginas = (tablaPagina_t*)malloc(sizeof(tablaPagina_t*)*MARCO);
	int j;
	for(j=0;j<MARCO;i++){
			tablaPaginas[i].pid = -1;
			tablaPaginas[i].marcoUtilizado = -1;
			tablaPaginas[i].bitPresencia = 0;
			tablaPaginas[i].bitModificacion = 0;
			tablaPaginas[i].bitUso = 0;
	}
}


int main(void) {

	crearLogs(string_from_format("umc_%d",getpid()),"Umc");
	log_info(activeLogger,"Soy umc de process ID %d.", getpid());

	crearMemoriaYTlbYTablaPaginas();

	//CAMBIAR, yo hice que umc sea cliente de nucleo, pero deberia ser servidor de nucleo!
	// Y FALTARIA QUE umc sea cliente de Swap

	servidorCPUyNucleo(); //OJO! A cada cpu hay que atenderla con un hilo

	realizarConexionASwap();
	escucharPedidosDeSwap();

	recibirComandos(); //Otro hilo?

	free(memoria);

	return 0;
}
