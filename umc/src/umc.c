/*
 * umc.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "umc.h"

struct timeval newEspera()
{
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}


void cargarCFG() {
	t_config* configUmc;
	configUmc = config_create("umc.cfg");
	config.puerto_swap = config_get_int_value(configUmc, "PUERTO_SWAP");
	config.puerto_umc_nucleo= config_get_int_value(configUmc, "PUERTO_UMC_NUCLEO");

	config.cantidad_marcos = config_get_int_value(configUmc, "CANTIDAD_MARCOS");
	config.tamanio_marco = config_get_int_value(configUmc, "TAMANIO_MARCO");

	config.entradas_tlb = config_get_int_value(configUmc, "ENTRADAS_TLB");
	config.retardo = config_get_int_value(configUmc, "RETARDO");
	config.ip_swap = config_get_string_value(configUmc, "IP_SWAP");
	config.puerto_cpu = config_get_int_value(configUmc, "PUERTO_CPU");
}

//0. Funciones auxiliares a las funciones Principales


int estaEnTlb(pedidoLectura_t pedido){
	int i;
	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			return 1;
		}
	}
	return 0;
}

int buscarEnTlb(pedidoLectura_t pedido){ //Repito codigo, i know, pero esta soluc no funciona para las dos, porque si se encuentra el pedido en tlb[0] y retornas 'i', "no estaria en tlb" cuando si
	int i;
	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			return i;
		}
	}
	return 0;
}

int existePidEnListadeTablas(int pid){
	(list_get(listaTablasPaginas, pid)!=NULL)?1:0; //Va a la posicion de la lista de las tablas de paginas. ==NULL no existe el elemento
	//(listaTablasPaginas[pid].nroPagina!=-1)?1:0;
}

int existePaginaBuscadaEnTabla(int pag, t_list* tablaPaginaBuscada){
	(list_get(tablaPaginaBuscada,pag))?1:0;   //EN VERDAD DEVUELVE NULL SI NO HAY NADA? ...
}

//char* buscarEnTablaMarcos(int marcoBuscado, pedidoLectura_t pedido){ //Ver si necesito o no el pedido, me suena que tenia que hacer algo
//	marco_t* marcoEnTabla = list_get(tablaMarcos,marcoBuscado);
//	return (marcoEnTabla->direccionContenido);
//}

char* buscarMarco(int marcoBuscado, pedidoLectura_t pedido){
	int pos = marcoBuscado * config.tamanio_marco;
	return memoria[pos];
}

int buscarPrimerMarcoLibre(){
	int i;
	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			return i;
		}
	}
	return -1;
}

int cantidadMarcosLibres(){
	int i;
	int c=0;

	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			c++;
		}
	}
	return c;
}

// FIN 0


// 1. Funciones principales
//
//typedef struct tlbStruct{
//		int pid,
//			pagina,
//			marcoUtilizado;
//	}tlb_t;

void agregarATlb(tablaPagina_t* pagina,int pidParam){

	int i;
	for(i=0;i<config.entradas_tlb;i++){
		if(tlb[i].pagina==-1){
			//Se encontro un espacio libre en la tlb, vamos a guardarlo ahi
			tlb[i].pagina = pagina->nroPagina;
			tlb[i].marcoUtilizado = pagina->marcoUtilizado;
			tlb[i].pid = pidParam;
			return;
		}
	}
	// TODO NO ENCONTRO ESPACIO EN LA TLB, ES HORA DE LA CACERIA...


}


char* buscarEnSwap(int marcoBuscado, pedidoLectura_t pedido){
	//TODO
}

char* agregarAMemoria(tablaPagina_t* paginaBuscada){
	//TODO
}

char* devolverPedidoPagina(pedidoLectura_t pedido){

	//SI ESTA EN TLB DEVUELVO

	if(estaEnTlb(pedido) && tlbHabilitada){
		log_info(activeLogger,"Se encontro en la Tlb el pid: %d, pagina: %d \n",pedido.pid,pedido.paginaRequerida);
		int pos = buscarEnTlb(pedido);

		char* contenido = malloc(pedido.cantBytes + 1);
		memcpy(contenido,memoria+tlb[pos].marcoUtilizado*config.tamanio_marco+pedido.offset,pedido.cantBytes);
		contenido[pedido.cantBytes]='\0';

		printf("marco tlb: %d \n", tlb[pos].marcoUtilizado);
		return contenido; //Provisorio, tiene que ser un SEND

		//send_w(cliente, contenido, 4);

	//HASTA ACA OK!

	}
	//SINO, ME FIJO QUE SEA VALIDA LA PETICION
	else{
		log_info(activeLogger,"No se encontro en la Tlb el pid: %d, pagina: %d. Se buscara en la Lista de tablas de paginas",pedido.pid,pedido.paginaRequerida);
		if(existePidEnListadeTablas(pedido.pid)){ //Si existe la tabla de paginas dentro de la lista
			t_list* tablaPaginaBuscada = list_get(listaTablasPaginas, pedido.pid);
			if(existePaginaBuscadaEnTabla(pedido.paginaRequerida,tablaPaginaBuscada)){ //Si la pagina existe dentro de la tabla particular
				tablaPagina_t* paginaBuscada = list_get(tablaPaginaBuscada, pedido.paginaRequerida);

	//SI ES VALIDA Y ESTA EN MEMORIA DEVUELVO Y AGREGO A TLB
				if(paginaBuscada->bitPresencia){

					log_info(activeLogger,"Se encontro la pagina y esta en memoria! Devolviendo pag:%d de pid:%d",pedido.paginaRequerida,pedido.pid);

					char* contenido = malloc(pedido.cantBytes + 1);
					memcpy(contenido,memoria+paginaBuscada->marcoUtilizado * config.tamanio_marco+pedido.offset,pedido.cantBytes);
					contenido[pedido.cantBytes]='\0';

					printf("marco tlb: %d \n", paginaBuscada->marcoUtilizado);


					agregarATlb(paginaBuscada,pedido.pid);

					//send_w(cliente, devolucion, 4);
					return contenido; //Provisorio, tiene que ser un SEND
		//HASTA ACA ANDA FENOMENO!
				}
	// SI ES VALIDA PERO NO ESTA EN MEMORIA, LA BUSCA EN SWAP Y TODO LA CARGO EN MEMORIA Y TLB Y RECIEN AHI LA DEVUELVOl, SI NO HAY PAGINAS DISPONIBLES: ALGORITMO DE SUSTITUCION DE PAGINAS
				else{
					char* devolucion = buscarEnSwap(paginaBuscada->marcoUtilizado,pedido);
					send_w(cliente, devolucion, 4);
					agregarAMemoria(paginaBuscada);
				}
			}
	// SI NO EXISTE LA PAGINA DENTRO DE LA TABLA DE PAG
			else{
				send_w(cliente, intToChar(HeaderNoExistePagina), 4);
			}
		}
	// SI NO EXISTE LA TABLA DE PAGINAS EN LA LISTA TOTAL DE PAGS
		else{
			send_w(cliente,intToChar(HeaderNoExisteTablaDePag), 4);
		}
	}
}


void almacenarBytesEnUnaPagina(int nroPagina, int offset, int tamanio, int buffer){
}

void finalizarPrograma(int idPrograma){
}

void inicializarPrograma(int idPrograma, int paginasRequeridas){
}


//FIN 1


//2. Funciones que se mandan por consola

void devolverTodasLasPaginas(){  //OK
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		t_list* unaTabla = malloc(sizeof(t_list));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size(unaTabla);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,j);

			printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",i,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
			log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",i,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
		}
	}
}

void devolverPaginasDePid(int pid){ //OK
	t_list* unaTabla = malloc(sizeof(t_list));
	int tamanioLista = list_size(listaTablasPaginas);

	if(pid<=tamanioLista){
		unaTabla = list_get(listaTablasPaginas,pid);
		int cantidadPaginasDeTabla = list_size(unaTabla);
		int i;

		for(i=0;i<cantidadPaginasDeTabla;i++){
			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,i);

			printf("Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
			log_info(dump, "Pid: %d, Pag: %d, Marco: %d, bitPresencia: %d, bitModificacion: %d, bitUso: %d \n",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,unaPagina->bitPresencia,unaPagina->bitModificacion,unaPagina->bitUso);
		}

	}
	else{
		printf("El pid supera la cantidad de tablas");
		log_info(dump, "El pid supera la cantidad de tablas");

	}

}

void devolverTodaLaMemoria(){ //OK

	int cantidadTablas = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<cantidadTablas;i++){

		t_list* unaTabla = malloc(sizeof(t_list));
		unaTabla = list_get(listaTablasPaginas,i);

		int cantidadPaginasDeTabla = list_size(unaTabla);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,j);
			//Hago un solo print f de las caracteristicas
			printf("Pid: %d, Pag: %d, Marco: %d, Contenido: ",i, unaPagina->nroPagina,unaPagina->marcoUtilizado);

			char* contenido = malloc(config.tamanio_marco+1);
			memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
			contenido[config.tamanio_marco]='\0';

			printf("%s",contenido);
			log_info(dump,"Pid: %d, Pag: %d, Marco: %d, Contenido: %s ",i, unaPagina->nroPagina,unaPagina->marcoUtilizado,contenido);

			printf("\n");
		}
	}
}

void devolverMemoriaDePid(int pid){ //OK
	t_list* unaTabla = malloc(sizeof(t_list));
	int tamanioLista = list_size(listaTablasPaginas);

	if(pid<=tamanioLista){
		unaTabla = list_get(listaTablasPaginas,pid);
		int cantidadPaginasDeTabla = list_size(unaTabla);
		int i;

		for(i=0;i<cantidadPaginasDeTabla;i++){
			tablaPagina_t* unaPagina = malloc(sizeof(tablaPagina_t));
			unaPagina = list_get(unaTabla,i);

			printf("Pid: %d, Pag: %d, Marco: %d, Contenido: ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado);

			char* contenido = malloc(config.tamanio_marco+1);
			memcpy(contenido,memoria+unaPagina->marcoUtilizado*config.tamanio_marco,config.tamanio_marco);
			contenido[config.tamanio_marco]='\0';

			printf("%s",contenido);
			log_info(dump, "Pid: %d, Pag: %d, Marco: %d, Contenido: %s ",pid,unaPagina->nroPagina,unaPagina->marcoUtilizado,contenido);

			printf("\n");


		}

	}
	else{
		printf("El pid supera la cantidad de tablas");
		log_info(dump, "El pid supera la cantidad de tablas");
	}
}

void fRetardo(){
	int nuevoRetardo=-1;
	printf("Ingrese el nuevo valor de Retardo en milisegundos: ");
	scanf(nuevoRetardo);
	config.retardo = nuevoRetardo*1000;
}
void dumpEstructuraMemoria(){ //Devuelve todas las tablas de paginas o de un solo pid
	int seleccion=-1;
	int pidDeseado;

	printf("0. Devolver todas las tablas |  1. Devolver las paginas de un proceso");
	scanf(seleccion);

	switch(seleccion){
		case 0:
			devolverTodasLasPaginas();

		case 1:
			printf("De que PID desea listar las paginas?");
			scanf(pidDeseado);
			devolverPaginasDePid(pidDeseado);
	}

}

void dumpContenidoMemoria(){ //Devuelve toda la memoria o solo la de un pid
	int seleccion=-1;
	int pidDeseado;

	printf("0. Devolver todas la Memoria|  1. Devolver la memoria de un proceso");
	scanf(seleccion);

	switch(seleccion){
		case 0:
			devolverTodaLaMemoria();

		case 1:
			printf("De que PID desea listar la memoria?");
			scanf(pidDeseado);
			devolverMemoriaDePid(pidDeseado);
	}
}
void flushTlb(){
	inicializarTlb();
}
void flushMemory(){ //Pone a todas las paginas bit de modificacion en 1
	int cantidadTablas = list_size(listaTablasPaginas);
	int i;
	for(i=0;i<cantidadTablas;i++){

		t_list* unaTabla = list_get(listaTablasPaginas,i);
		int cantidadPaginasDeTabla = list_size(unaTabla);
		int j;

		for(j=0;j<cantidadPaginasDeTabla;j++){

			tablaPagina_t* unaPagina = list_get(unaTabla,j);
			unaPagina->bitModificacion=1;
		}
	}
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

// 3. Inicializar estructura de UMC
void inicializarTlb(){
	int i;
	for(i = 0; i<config.entradas_tlb; i++){
		tlb[i].pid=-1;
		tlb[i].pagina=-1;
		tlb[i].marcoUtilizado=-1;
	}
}

void crearMemoriaYTlbYTablaPaginas(){

	//Creo memoria y la relleno
	tamanioMemoria = config.cantidad_marcos * config.tamanio_marco;
	memoria = malloc(tamanioMemoria);
	memset(memoria,'\0',tamanioMemoria);
	log_info(activeLogger,"Creada la memoria.\n");

	//Relleno TLB
	tlb = malloc(config.entradas_tlb * sizeof(tlb_t));
	inicializarTlb();
	log_info(activeLogger,"Creada la TLB y rellenada con ceros (0).\n");

	//Creo vector de marcos ocupados y lo relleno
	vectorMarcosOcupados = malloc(sizeof(int) * config.cantidad_marcos);
	log_info(activeLogger,"Creado el vector de marcos ocupados \n");

	memset(vectorMarcosOcupados,0,sizeof(int) * config.cantidad_marcos);

}




// FIN 3

// 4. Procesar headers

int reservarPagina(int cantPaginasPedidas, int pid){

	if(cantidadMarcosLibres()>=cantPaginasPedidas){ //Si alcanzan los marcos libres...

		int i;
		for(i=0;i<cantPaginasPedidas;i++){

			tablaPagina_t *nuevaPag = malloc(sizeof(tablaPagina_t));

			int unMarcoNuevo = buscarPrimerMarcoLibre();
			vectorMarcosOcupados[unMarcoNuevo]=1; //Lo marco como ocupado
			printf("Marco seleccionado numero: %d (deberia ser 4, 5 y 6)\n", unMarcoNuevo);

			nuevaPag->nroPagina = i;
			nuevaPag->marcoUtilizado = unMarcoNuevo;
			nuevaPag->bitPresencia=1;
			nuevaPag->bitModificacion=0;
			nuevaPag->bitUso=1;

			t_list* tablaPag = list_get(listaTablasPaginas,pid);
			list_add_in_index(tablaPag,i,nuevaPag);
		}
		return 1;
	}
	else return 0; // No hay marcos libres
}

void procesarHeader(int cliente, char *header){
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger,"Llego un mensaje con header %d\n",charToInt(header));
	clientes[cliente].atentido=true;

	switch(charToInt(header)) {

	case HeaderError:
		log_error(activeLogger,"Header de Error\n");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger,"Llego un handshake\n");
		payload_size=1;
		payload = malloc(payload_size);
		read(clientes[cliente].socket , payload, payload_size);
		log_debug(bgLogger,"Llego un mensaje con payload %d\n",charToInt(payload));
		if ( (charToInt(payload)==SOYCPU) || (charToInt(payload)==SOYNUCLEO) ){
			log_debug(bgLogger,"Es un cliente apropiado! Respondiendo handshake\n");
			clientes[cliente].identidad = charToInt(payload);
			send(clientes[cliente].socket, intToChar(SOYUMC), 1, 0);

		}
		else {
			log_error(activeLogger,"No es un cliente apropiado! rechazada la conexion\n");
			log_warning(activeLogger,"Se quitará al cliente %d.",cliente);
			quitarCliente(cliente);
		}
		free(payload);
		clientes[cliente].atentido=false;
		break;

		case HeaderReservarEspacio:

			pedidoPaginaPid = recv_waitall_ws(cliente, sizeof(int));
			pedidoPaginaTamanioContenido = recv_waitall_ws(cliente, sizeof(int));
			//ES NECESARIO TENER EL PID DEL PROCESO Q NUCLEO QUIERE GUARDAR EN MEMORIA? SI: RECIBIR INT  NO: RECIBIR NADA
			log_info(activeLogger,"Nucleo me pidio memoria");

			int cantPaginasPedidas = ((float)charToInt(pedidoPaginaTamanioContenido) + config.tamanio_marco - 1) / config.tamanio_marco; //A+B-1 / B
			int pid = charToInt(pedidoPaginaPid);

			//TODO Primero: if swap tiene espacio..

			if(reservarPagina(cantPaginasPedidas,pid)){
				send_w(cliente, headerToMSG(HeaderTeReservePagina), 1);
			}
			else{
				send_w(cliente, headerToMSG(HeaderErrorNoHayPaginas), 1);
			};


		case HeaderPedirContenidoPagina:
			log_info(activeLogger,"Se recibio pedido de pagina, por CPU");
			pedidoLectura_t pedido;
			pedido.pid = recv_waitall_ws(cliente, sizeof(int));
			pedido.paginaRequerida = recv_waitall_ws(cliente, sizeof(int));
			pedido.offset = recv_waitall_ws(cliente, sizeof(int));
			pedido.cantBytes = recv_waitall_ws(cliente, sizeof(int));
			devolverPedidoPagina(pedido);

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




// FIN 4


void test(){

	vectorMarcosOcupados[0]=1;
	vectorMarcosOcupados[1]=1;
	vectorMarcosOcupados[2]=1;

	int marcoNuevo = buscarPrimerMarcoLibre();
	vectorMarcosOcupados[marcoNuevo]=1; //Lo marco como ocupado

	printf("El primer marco libre deberia ser el 3 y es: %d \n", marcoNuevo);
	printf("Y ahora su contenido deberia ser 1: %d \n\n", vectorMarcosOcupados[marcoNuevo]);

	printf("Contenido vector en pos 0: %d \n", vectorMarcosOcupados[0]);
	printf("Contenido vector en pos 1: %d \n", vectorMarcosOcupados[1]);
	printf("Contenido vector en pos 2: %d \n", vectorMarcosOcupados[2]);
	printf("Contenido vector en pos 3: %d \n", vectorMarcosOcupados[3]);
	printf("Contenido vector en pos 4: %d \n", vectorMarcosOcupados[4]);
	printf("Contenido vector en pos 5: %d \n", vectorMarcosOcupados[5]);
	printf("Contenido vector en pos 6: %d \n", vectorMarcosOcupados[6]);
	printf("Contenido vector en pos 7: %d \n", vectorMarcosOcupados[7]);
	printf("Contenido vector en pos 8: %d \n", vectorMarcosOcupados[8]);
	printf("Contenido vector en pos 9: %d \n", vectorMarcosOcupados[9]);


	printf("Pasamos al test de memoria \n \n");

	int test = reservarPagina(3,5);

	if(test){
		tabla5 = list_get(listaTablasPaginas, 5);

		tablaPagina_t* pagina0Tabla5 = list_get(tabla5,0);
		tablaPagina_t* pagina1Tabla5 = list_get(tabla5,1);
		tablaPagina_t* pagina2Tabla5 = list_get(tabla5,2);

		printf("Agarramos la tabla de paginas en las posicion 5. \n");
		printf("Y deberia tener 3 paginas dentro, coincide con cant: %d \n", list_size(tabla5));
		printf("En la posicion 0 estaria la pagina 0 con marco 4, coincide con: pagina:%d, marco: %d \n", pagina0Tabla5->nroPagina, pagina0Tabla5->marcoUtilizado);
		printf("En la posicion 0 estaria la pagina 1 con marco 5, coincide con: pagina:%d, marco: %d \n", pagina1Tabla5->nroPagina, pagina1Tabla5->marcoUtilizado);
		printf("En la posicion 0 estaria la pagina 2 con marco 6, coincide con: pagina:%d, marco: %d \n", pagina2Tabla5->nroPagina, pagina2Tabla5->marcoUtilizado);
	}
	else{
		printf("No hay paginas disponibles");
	}

	memoria[5*config.tamanio_marco]='a';
	memoria[5*config.tamanio_marco+1]='b';
	memoria[5*config.tamanio_marco+2]='c';
	memoria[5*config.tamanio_marco+3]='d';
	memoria[5*config.tamanio_marco+11]='d';

	memoria[6*config.tamanio_marco]='a';
	memoria[6*config.tamanio_marco+1]='b';
	memoria[6*config.tamanio_marco+2]='c';

	devolverTodaLaMemoria();

	printf("---- De solo pid 5 --- \n");
	devolverMemoriaDePid(5);

	printf(" -------------------------------------------  \n \n");
	printf("Haciendo un pedido de pagina \n");

	pedidoLectura_t pedido;
	pedido.pid = 5;
	pedido.paginaRequerida = 1;
	pedido.offset = 2;
	pedido.cantBytes = 2;

	printf("Simulamos que esta en TLB \n");
	tlb[3].pid=5;
	tlb[3].pagina=1;
	tlb[3].marcoUtilizado= 5;

	char* respuesta = devolverPedidoPagina(pedido);
	printf("Respuesta1 deberia ser 'cd' y es: %s  \n",respuesta); //Original: abcd Empieza de la posicion 2 y lee 2 bytes

	printf("--***---\n");

	printf("Reinicio la tlb y no deberia encontrar, paso a buscarla en lista tablas \n");
	inicializarTlb();
	char* respuesta2 = devolverPedidoPagina(pedido);
	printf("Respuesta2 deberia ser 'cd' y es: %s  \n",respuesta2);
	printf("Y se deberia haber agregado a tlb.. \n");
	printf("Tlb deberia ser (5,1,5) y es : Pid: %d, Pagina: %d, Marco: %d \n", tlb[0].pid, tlb[0].pagina, tlb[0].marcoUtilizado);
	printf("--***---\n");

}

void finalizar() {
	destruirLogs();
	log_destroy(dump);
	list_destroy(listaTablasPaginas);
	free(memoria);
}


int main(void) {

	cargarCFG();

	crearLogs("Umc","Umc");

	dump = log_create("dump","UMC",false,LOG_LEVEL_INFO);

	log_info(activeLogger,"Soy umc de process ID %d.\n", getpid());

	int k;
	listaTablasPaginas = list_create();

	for(k=0;k<config.cantidad_marcos;k++){  //COMO MAXIMO ES LA CANTIDAD DE MARCOS, considerando q como minimo una tabla tiene 1 pag
		t_list* tablaPaginas = list_create();
		list_add(listaTablasPaginas,tablaPaginas);
	}

	crearMemoriaYTlbYTablaPaginas();

	test();

	servidorCPUyNucleoExtendido();
//	servidorCPUyNucleo();
	//pthread_create(&conexCpu, NULL, (void*) servidorCPUyNucleoExtendido, NULL);
	//pthread_create(&conexSwap, NULL, (void*) conexionASwap, NULL);
	//pthread_create(&conexNucleo, NULL, (void*) servidorCPUyNucleo, NULL); //OJO! A cada cpu hay que atenderla con un hilo

	//recibirComandos(); //Otro hilo?

	finalizar();

	return 0;
}

































// 5.Server de los cpu y de nucleo

void servidorCPUyNucleoExtendido(){

	struct timeval espera = newEspera();
	int i;
	char header[1];

	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
				&tamanioDireccionCPU, &activadoCPU);
	configurarServidorExtendido(&socketNucleo, &direccionNucleo, config.puerto_umc_nucleo,
					&tamanioDireccionNucleo, &activadoNucleo);

	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones ...");

	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);
		FD_SET(socketNucleo, &socketsParaLectura);

		mayorDescriptor = (socketNucleo>socketCPU) ? socketNucleo : socketCPU;
		incorporarClientes();

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, &espera);

		if (tieneLectura(socketNucleo))
			procesarNuevasConexionesExtendido(&socketNucleo);

		if (tieneLectura(socketCPU))
			procesarNuevasConexionesExtendido(&socketCPU);

		for (i = 0; i < getMaxClients(); i++) {
			if (tieneLectura(clientes[i].socket)) {
				if (read(clientes[i].socket, header, 1) == 0) {
					log_error(activeLogger,
					"Un cliente se desconectó.");
					quitarCliente(i);
				} else
					procesarHeader(i, header);
				}
			}

			//Hacer algo?
	}
}


void servidorCPUyNucleo(){

	int mayorDescriptor, i;
	struct timeval espera = newEspera(); 		// Periodo maximo de espera del select
	char header[1];

	crearLogs("Umc","Umc");

	configurarServidor(config.puerto_umc_nucleo);
	inicializarClientes();
	log_info(activeLogger,"Esperando conexiones ...");

	while(1){
		mayorDescriptor = incorporarSockets();
		select( mayorDescriptor + 1 , &socketsParaLectura , NULL , NULL , &espera);

		if (tieneLectura(socketNuevasConexiones))
			procesarNuevasConexiones();
	}
}

int getHandshake()
{
	char* handshake = recv_nowait_ws(cliente,1);
	return charToInt(handshake);
}

// FIN 5


// 6. Conexion a Swap
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
	direccion = crearDireccionParaCliente(config.puerto_swap,config.ip_swap);  //CAMBIAR ESTO DE IP
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
		if (cliente!=0){ // Solo si esta conextado
			//header = recv_waitall_ws(cliente,sizeof(char)); ESTO NO ME PERMITE CHEQUEAR SI SE DESCONECTO!!
			header = malloc(1);
			int bytesRecibidos = recv(cliente, header, 1, MSG_WAITALL);
			if (bytesRecibidos <= 0){
				 printf("SWAP se desconecto\n");
				 close(cliente);
				 cliente=0;
				 return;
			}
			else
				procesarHeader(cliente,header);

			free(header);
		}
	}
}

void conexionASwap(){ //Creada para unir las dos funciones y crear un hilo
	realizarConexionASwap();
	escucharPedidosDeSwap();

}

// FIN 6




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
