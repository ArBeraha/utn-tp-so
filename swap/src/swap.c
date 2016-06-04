/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "swap.h"

// *******************************************************FUNCIONES UTILES**********************************************************
/*---------------INICIALIZACION SWAP----------------*/
void cargarCFG() {
	t_config* configSwap;
	configSwap = config_create("swap.cfg");
	config.puerto_escucha = config_get_int_value(configSwap, "PUERTO_ESCUCHA");
	config.nombre_swap = config_get_string_value(configSwap, "NOMBRE_SWAP");
	config.cantidad_paginas =  config_get_int_value(configSwap, "CANTIDAD_PAGINAS");
	config.tamanio_pagina =  config_get_int_value(configSwap, "TAMANIO_PAGINA");
	config.retardo_compactacion = config_get_int_value(configSwap, "RETARDO_COMPACTACION");
	config.retardo_acceso =  config_get_int_value(configSwap, "RETARDO_ACCESO");

	log_info(activeLogger, "Archivo de configuracion cargado.\n");
}

void crear_archivo() {

	char* ddComand; // comando a mandar a consola para crear archivo mediante dd
	ddComand = string_new(); //comando va a contener a dd que voy a mandar a consola para que cree el archivo

	string_append(&ddComand, "dd if=/dev/zero of="); //crea archivo input vacio
	string_append(&ddComand, config.nombre_swap); //con el nombre de la swap
	string_append(&ddComand, " bs="); //defino tamaño del archivo (de la memoria swap)
	string_append(&ddComand, string_itoa(config.tamanio_pagina)); //Lo siguiente no va ya que ahora mi memoria se divide en paginas, no bytes
	string_append(&ddComand, " count=");
	string_append(&ddComand, string_itoa(config.cantidad_paginas)); //cuyo tamaño va a ser igual al tamaño de las paginas*cantidad de paginas
	printf("%s\n", ddComand);
	system(ddComand); //ejecuto comando

	free(ddComand);
	log_info(activeLogger, "Archivo %s creado\n", config.nombre_swap);


}

void conectar_umc(){

	configurarServidor(config.puerto_escucha);
	log_info(activeLogger,"Esperando conexion de umc ...\n");

	t_cliente clienteData;
	clienteData.addrlen=sizeof(clienteData.addr);
	clienteData.socket=accept(socketNuevasConexiones,(struct sockaddr*)&clienteData.addr,(socklen_t*)&clienteData.addrlen);
	printf("Nueva conexion , socket %d, ip is: %s, puerto: %d \n", clienteData.socket, inet_ntoa(clienteData.addr.sin_addr),
			ntohs(clienteData.addr.sin_port));
	cliente=clienteData.socket;

	log_info(activeLogger, "Conexion a UMC correcta :)\n.");
}

void inicializar(){
	crearLogs("SWAP", "SWAP",0);
	crearLogs(string_from_format("swap_%d", getpid()), "SWAP",0);
	log_info(activeLogger, "Soy SWAP de process ID %d.\n", getpid());

	cargarCFG();
	crear_archivo();
}
/*------------------------------------------------*/

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

int espaciosDisponibles (t_bitarray* unEspacio){
  int i;
  int espacioSinUtilizar=0;
  for(i=0; i<config.cantidad_paginas; i++)
  {
	  if (bitarray_test_bit(unEspacio, i)==0) espacioSinUtilizar++ ;
  }
  return espacioSinUtilizar;
}

int espaciosUtilizados (t_bitarray* unEspacio){
  int i;
  int espacioUtilizado=0;
  for(i=0; i<config.cantidad_paginas; i++)
  {
	  if (bitarray_test_bit(unEspacio, i)==0) espacioUtilizado++ ;
  }
  return espacioUtilizado;
}


void limpiarPosiciones (t_bitarray* unEspacio, int posicionInicial, int tamanioProceso){
	int i=0;
	for(i=posicionInicial; i < posicionInicial+tamanioProceso; i++) bitarray_clean_bit(unEspacio, i);
}

void setearPosiciones (t_bitarray* unEspacio, int posicionInicial, int tamanioProceso){
	int i=0;
	for(i=posicionInicial; i < posicionInicial+tamanioProceso; i++) bitarray_set_bit(unEspacio, i);
}


//Comprueba si hay fragmentacion externa
int hayQueCompactar(int paginasAIniciar) {
	int cantidadDePaginas = config.cantidad_paginas;
	int i=0;
	int flag = 1;
	int marcos;
	for (i = 0; i < cantidadDePaginas; i++) {
		if(bitarray_test_bit(espacio,i)==0) marcos++;
		else marcos=0;
		if (marcos>= paginasAIniciar) {
			flag = 0;
		}
	}
	return flag;
}
/*************************Solo para test***********************************************/

int testArrayOcupado(t_bitarray* unEspacio, int posicionInicial, int tamanioProceso )
{
	int i=0;
	int ocupado=0;
	int flag=0;
	for(i=posicionInicial; i < posicionInicial+tamanioProceso; i++)
	{
		if (bitarray_test_bit(unEspacio, i)) ocupado++;
	}
	if (ocupado==tamanioProceso) flag=1;

    return flag;

}


int estaEnArray(t_infoProceso* unDatoProceso)
{
	int estado=0;
	int i=0;
	int espacioOcupado=0;
	for(i=unDatoProceso->posPagina; i<(unDatoProceso->cantidadDePaginas+unDatoProceso->posPagina); i++)
	{
		if(bitarray_test_bit(espacio,i)) espacioOcupado++;
	}
	if(espacioOcupado==unDatoProceso->cantidadDePaginas) estado=1;
	return estado;

}

int primerEspacioLibre(t_bitarray* unEspacio)
{

    int aux=0;
    int i=0;
	for(i=0; i<config.cantidad_paginas; i++)
	  {
		  if (bitarray_test_bit(unEspacio, i)==0)
		  {
			  aux=i;
			  break;
		  }

	  }
	return aux;
}


/*********************************************************************************/
int coincideElPID(t_infoProceso* unDatoProceso, int unPID)
{
	return (unDatoProceso->pid == unPID);
}

int coincideElMarco(t_infoProceso* unDatoProceso, int unMarco)
{
	return (unDatoProceso->posPagina == unMarco);
}

int estaElProceso(int unPid)
{
	t_infoProceso* datoProceso;
		int i=0;
		int cantidadProcesos = list_size(espacioUtilizado);
		int estado = 0;
		while (i < cantidadProcesos && estado==0)
		{
			datoProceso= (t_infoProceso*)list_get(espacioUtilizado,i);
		    if(coincideElPID(datoProceso,unPid)) estado = 1;
		    i++;
		}
	    return estado;
}

t_infoProceso* buscarProceso(int unPid)
{
	t_infoProceso* datoProceso=malloc(sizeof(t_infoProceso));
	int i=0;
	int cantidadProcesos = list_size(espacioUtilizado);
	int estado = 0;
	while (i < cantidadProcesos && estado==0)
	{
		datoProceso= (t_infoProceso*)list_get(espacioUtilizado,i);
	    if(coincideElPID(datoProceso,unPid)) estado = 1;
	    i++;
	}
    return datoProceso;
}

int buscarMarcoInicial(int unPid) {
	t_infoProceso* datoProceso = buscarProceso(unPid);
	return datoProceso->posPagina;
}

t_infoProceso* buscarProcesoAPartirDeMarcoInicial(int marcoInicial)
{
	int i=0;
	int cantidadProcesos=list_size(espacioUtilizado);
	int estado=0;
	t_infoProceso* datoProceso=malloc(sizeof(t_infoProceso));
	while (i< cantidadProcesos && estado==0)
	{
		datoProceso= (t_infoProceso*)list_get(espacioUtilizado,i);
		if(coincideElMarco(datoProceso,marcoInicial)) estado=1;
		i++;
	}
	return datoProceso;
}

void sacarElemento(int unPid)
{
	t_infoProceso* datoProceso;
		int i=0;
		int cantidadProcesos = list_size(espacioUtilizado);
		for(i=0; i<cantidadProcesos; i++)
		{
			datoProceso= (t_infoProceso*)list_get(espacioUtilizado,i);
		    if(coincideElPID(datoProceso,unPid))
		    {
		    	list_remove(espacioUtilizado,i);
		    	break;
		    }
		}

}

//NO SE USA MAAAAAAAAAAAAAAAAAAAS CREOOOOO (SINDROME DE DIOGENES)

////Funcion que busca en la lista de utilizados el proceso con el marco igual o mas proximo (mayor) al numero que se le pasa
//t_infoProceso* elemMIMenor (marcoAComparar) {
//	int cantElementos = list_size(espacioUtilizado);
//	t_infoProceso* elem1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
//	int i;
//	for (i = 0; i < cantElementos; i++) {
//		elem1 = list_get(espacioUtilizado, i);
//		if (elem1->posPagina >= marcoAComparar) {
//			break;
//		}
//	}
//	int j;
//	for (j = 0; j < cantElementos; j++) {
//		t_infoProceso* elem2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
//		elem2 = list_get(espacioUtilizado, j);
//		if (elem2->posPagina >= marcoAComparar && elem1->posPagina > elem2->posPagina) {
//			elem1 = elem2;
//		}
//	}
//	return elem1;
//}




void compactar() //TODO MIRAR RESULTADO EN TESTCOMPACTACIONSWAP3()
{
	int i=0;
	int espaciosLibres=0;
	int posActual=0;
	int nuevaPosicion;
	t_infoProceso* procesoActual;
	for(i=0;i<config.cantidad_paginas;i++)
	{
		if (bitarray_test_bit(espacio, i)==0) espaciosLibres++ ;
		if (espaciosLibres!=0 && bitarray_test_bit(espacio,i)==1)
		{
			posActual=i;
			procesoActual=buscarProcesoAPartirDeMarcoInicial(posActual);
			nuevaPosicion= (posActual- espaciosLibres);
			procesoActual->posPagina = nuevaPosicion; //Para la lista
			modificarArchivo(posActual,procesoActual->cantidadDePaginas,nuevaPosicion); //Para el archivo
			limpiarPosiciones(espacio,posActual,procesoActual->cantidadDePaginas); //Para el array
			setearPosiciones(espacio,nuevaPosicion,procesoActual->cantidadDePaginas); //Para el array
			i=posActual+(procesoActual->cantidadDePaginas)-1;


		}
	}
	//sleep(config.retardo_compactacion); //TODO DESCOMENTAR PARA CUANDO SE PRUEBE EN SERIO
	   log_info(activeLogger, "Compactación finalizada.");

}

void modificarArchivo (int marcoInicial, int cantMarcos, int nuevoMarcoInicial)
{
	FILE *archivoSwap;
	archivoSwap = fopen(config.nombre_swap, "r+");
	if (archivoSwap == NULL) {
		printf("Error al abrir el archivo para escribir\n");
	}
	//Leo en el archivo los marcos que le correspondian al proceso que estamos modificando
	char* buffer = malloc(config.tamanio_pagina*cantMarcos);
	fseek(archivoSwap, marcoInicial, SEEK_SET);
	fread(buffer, config.tamanio_pagina, cantMarcos, archivoSwap);
	//Lleno de ceros el archivo en la parte que acabo de leer
	char* texto = malloc(config.tamanio_pagina*cantMarcos);
	int i;
	for (i = 0; i < config.tamanio_pagina*cantMarcos; i++) {
		texto[i] = '\0';
	}
	fseek(archivoSwap, marcoInicial, SEEK_SET);
	fwrite(texto, config.tamanio_pagina*cantMarcos, 1, archivoSwap);
	//Escribo lo que lei del proceso en los nuevos marcos que le asignamos
	fseek(archivoSwap, nuevoMarcoInicial, SEEK_SET);
	fwrite(buffer, config.tamanio_pagina, cantMarcos, archivoSwap);
	fclose(archivoSwap);
}



//************************************FUNCIONES PRINCIPALES DE SWAP*********************************************************


void asignarEspacioANuevoProceso(int pid, int paginasAIniciar){

	if (paginasAIniciar <= espacioDisponible)
	{
	   //Me fijo si hay fragmentacion para asi ver si necesito compactar
	   if (hayQueCompactar(paginasAIniciar)) {
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
	int i;
	int totalMarcos=0;
	int marcoInicial=0;
	int aux=0;
	//Recorro el bitarray hasta que encuentro un hueco ocupado
	for (i = 0; i < config.cantidad_paginas; i++)
	{
		//Recorro el bitarray hasta que encuentro un hueco
		//BUSCO MIENTRAS ESTE VACIO, CUANDO ENCUENTRO OCUPADO PONGO EN 0 LOS MARCOS ENCONTRADOS, PERO MIENTRAS ME FIJO QUE EL SIGUIENTE
		//PODRIA SER MI MARCO INICIAL
		if(bitarray_test_bit(espacio,i)==0) totalMarcos++;
		if(bitarray_test_bit(espacio,i)) {
			aux=i;
			marcoInicial = (++aux);
			totalMarcos=0;
		}
		printf("Cantidad de marcos %d\n", totalMarcos);
        //Si ese hueco me permite alojar las paginas
		if (totalMarcos>= paginasAIniciar)
		{
			//alojo el proceso (marco como ocupado)
			setearPosiciones(espacio,marcoInicial,paginasAIniciar);
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
						"El programa %d cuyo Marco Inicial es:%d y su Tamanio es:%d paginas fue Iniciado correctamente.",
						pid, proceso->posPagina,
						proceso->cantidadDePaginas );
				send_w(cliente, headerToMSG(HeaderProcesoAgregado), 1);
				return;

		     }

		}
	}


	printf("Hay que compactar\n");

	send_w(cliente, headerToMSG(HeaderHayQueCompactar), 1);

	return;
  }




void leerPagina(int pid, int paginaALeer) {


char* buffer = malloc(config.tamanio_pagina + 1);

//Abro el archivo de Swap
	archivoSwap = fopen(config.nombre_swap, "r");
	if (archivoSwap == NULL) {
		printf("Error al abrir el archivo para leer\n");
	}

//Me posiciono en la página que quiero leer y guardo lo que leo en el buffer
	int marcoInicial = buscarMarcoInicial(pid);
	int marcoALeer = (marcoInicial + paginaALeer);
	int exitoAlLeer = fseek(archivoSwap, marcoALeer, SEEK_SET);
	fread(buffer, config.tamanio_pagina, 1, archivoSwap);
	fclose(archivoSwap);
	usleep(config.retardo_acceso);
	//mirar si no se puede leer
	log_info(activeLogger, "El programa %d cuyo Marco Inicial es:%d de Tamanio:%d .Contenido:%s. Lectura realizada correctamente.",
			pid, marcoInicial , string_length(buffer), buffer);
	if (exitoAlLeer == 0) // si se leyo bien la pagina
			{
		send_w(cliente, headerToMSG(HeaderLecturaCorrecta), 1);

		//Envio lo leído a memoria
		send_w(cliente, (void*) buffer, config.tamanio_pagina);
		buffer[config.tamanio_pagina] = '\0';
		printf("Lectura exitosa : %s\n", buffer);

	} else {
		send_w(cliente,headerToMSG(HeaderLecturaErronea), 1);
		printf("Error al intentar leer\n");

	}

//Libero el malloc del buffer
	free(buffer);
}


void escribirPagina(int pid, int paginaAEscribir, int tamanio) {
	//Abro el archivo de Swap
	archivoSwap = fopen(config.nombre_swap, "r+");
	if (archivoSwap == NULL) {
		printf("Error al abrir el archivo para escribir\n");
	}
	char* texto = malloc(config.tamanio_pagina);
	recv(cliente, (void*) texto, tamanio, MSG_WAITALL);
	//Al buffer que me envian para escribir lo lleno de ceros hasta completar el tamaño de página
	int i;
	for (i = tamanio; i < config.tamanio_pagina; i++) {
		texto[i] = '\0';
	}
	//Me posiciono en la página que quiero escribir y escribo
	int marcoInicial = buscarMarcoInicial(pid);
	int marcoAEscribir = (marcoInicial + paginaAEscribir); //TODO: marcoAEscribir señala el final del marco
	fseek(archivoSwap, marcoAEscribir, SEEK_SET);
	printf("texto:%s\n", texto);
	int exitoAlEscribir = fwrite(texto, config.tamanio_pagina, 1, archivoSwap);

	fclose(archivoSwap);
	usleep(config.retardo_acceso);

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
	if (estaElProceso(pid))
	{
	 proceso = buscarProceso(pid);
     //Actualizo la variable espacioDisponible
	 espacioDisponible += proceso->cantidadDePaginas;
	 limpiarPosiciones (espacio, proceso->posPagina, proceso->cantidadDePaginas);
	 sacarElemento(pid);
     usleep(config.retardo_acceso);
	 printf("Proceso eliminado exitosamente\n");
	 log_info(activeLogger, "El programa %d - Pagina Inicial:%d Tamanio:%d Eliminado correctamente.",
	 pid, proceso->posPagina, proceso->cantidadDePaginas);
	 send_w(cliente, headerToMSG(HeaderProcesoEliminado), 1);
	 free(proceso);
	}
	else
	{
		printf("Proceso no encontrado\n");
		log_error(activeLogger, "El proceso no fue encontrado, error al eliminar el proceso %d",pid);
		send_w(cliente,headerToMSG(HeaderProcesoNOEliminado),1);

	}

}

void esperar_peticiones(){

	char* header;
	while (1){
		header=recv_waitall_ws(cliente,1);
		procesarHeader(cliente,header); //Incluye deserializacion
    }
}

void finalizar(){ //TODO irian mas cosas, tipo free de mallocs y esas cosas

	log_info(activeLogger,"Proceso Swap terminado");
}


//**************************************************MAIN SWAP*****************************************************************

int main()
{
	inicializar();

	espacioDisponible = config.cantidad_paginas; //Para manejar la asignacion de paginas a procesos

	/* bitarray manejo de paginas */
	int tamanio = (config.cantidad_paginas/8);
	char* data = malloc(tamanio+1);
	strcpy(data, "\0");
	espacio = bitarray_create(data,tamanio);
	espacioUtilizado = list_create();
	//marco libres todos las posiciones del array
    limpiarPosiciones (espacio,0,config.cantidad_paginas);

    testSwapDeBitArray1();
    testSwapDeBitArray2();
    testSwapDeCompactacion3();
    testSwapDeCompactacion4();
    testFinalizarProceso1();
    testFinalizarProceso2();
    testFinalizarProceso3();
    testAgregarProceso1();
    testAgregarProceso2(); //TODO FALTA agregar FUNCIONANDO
    testLectura2();
    testLectura3();  //TODO PROBLEMAS CON LEER ARCHIVO, NO SE LEE LO QUE QUIERO
    testLectura4(); //TODO PROBLEMAS CON AGREGAR PROCESO

    espacioDisponible = config.cantidad_paginas;
    limpiarPosiciones (espacio,0,config.cantidad_paginas);
    list_clean(espacioUtilizado);

	//SOCKETS
	conectar_umc();

	esperar_peticiones();

	finalizar();

	return 0;
}


