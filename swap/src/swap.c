/*
 * swap.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "swap.h"


void inicializar() {
	crearLogs("SWAP", "SWAP", 0);
	crearLogs(string_from_format("swap_%d", getpid()), "SWAP", 0);
	log_info(activeLogger, "Soy SWAP de process ID %d.\n", getpid());
	cargarCFG();
	crear_archivo();
	abrirArchivo();
	configurarBitarray();
	espacioDisponible = config.cantidad_paginas;
}
void cargarCFG() {
	t_config* configSwap;
	configSwap = config_create("swap.cfg");
	config.puerto_escucha = config_get_int_value(configSwap, "PUERTO_ESCUCHA");
	config.nombre_swap = config_get_string_value(configSwap, "NOMBRE_SWAP");
	config.cantidad_paginas = config_get_int_value(configSwap,
			"CANTIDAD_PAGINAS");
	config.tamanio_pagina = config_get_int_value(configSwap, "TAMANIO_PAGINA");
	config.retardo_compactacion = config_get_int_value(configSwap,
			"RETARDO_COMPACTACION");
	config.retardo_acceso = config_get_int_value(configSwap, "RETARDO_ACCESO");
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
void conectar_umc() {
	configurarServidor(config.puerto_escucha);
	log_info(activeLogger, "Esperando conexion de umc ...\n");
	t_cliente clienteData;
	clienteData.addrlen = sizeof(clienteData.addr);
	clienteData.socket = accept(socketNuevasConexiones,
			(struct sockaddr*) &clienteData.addr,
			(socklen_t*) &clienteData.addrlen);
	printf("Nueva conexion , socket %d, ip is: %s, puerto: %d \n",
			clienteData.socket, inet_ntoa(clienteData.addr.sin_addr),
			ntohs(clienteData.addr.sin_port));
	cliente = clienteData.socket;
	log_info(activeLogger, "Conexion a UMC correcta :)\n.");
}

void operacionCodigo(){

}

void procesarHeader(int cliente, char* header) {
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error.");
		break;

	case HeaderHandshake:
		operacionHandshake();
		break;

	case HeaderOperacionIniciarProceso:
		operacionIniciarProceso();
		break;

	case HeaderOperacionLectura:
		operacionLectura();
		break;

	case HeaderOperacionEscritura:
		operacionEscritura();
		break;

	case HeaderOperacionFinalizarProceso:
		operacionFinalizar();
		break;

	default:
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		exit(EXIT_FAILURE);
		break;
	}
}
void esperar_peticiones() {
	char* header;
	while (1) {
		header = recv_waitall_ws(cliente, 1);
		procesarHeader(cliente, header);
	}
}
void abrirArchivo() {
	if ((fd = open(config.nombre_swap, O_RDWR)) == -1) {
		perror("open");
		exit(1);
	}
	// archivoEnPaginas es el primer multiplo de paginas DEL SISTEMA en las que entra el archivo
	archivoEnPaginas = ((int) ((config.cantidad_paginas * config.tamanio_pagina)
			/ getpagesize()) + 1) * getpagesize();
	archivo = mmap((void*) 0, archivoEnPaginas, PROT_WRITE, MAP_SHARED, fd, 0);
}
void cerrarArchivo() {
	munmap(archivo, archivoEnPaginas);
	close(fd);
}
void finalizar() {
	cerrarArchivo();
	bitarray_destroy(espacio);
	list_destroy(espacioUtilizado);
	log_info(activeLogger, "Proceso Swap terminado");
}
// Funciones de comunicacion con umc
void operacionHandshake(){
	char* handshake = malloc(1);
	handshake = recv_waitall_ws(cliente, 1);
	if (charToInt(handshake) == SOYUMC) {
		log_debug(bgLogger, "Es UMC! Respondiendo handshake");
		send_w(cliente, intToChar(SOYSWAP), 1);
	} else {
		log_error(activeLogger, "No es UMC! rechazada la conexion");
	}
	free(handshake);
}
void operacionIniciarProceso(){
	log_info(activeLogger, "Se recibio inicializacion");
	char* serialPID = malloc(sizeof(int));
	char* serialPagina = malloc(sizeof(int));
	serialPID = recv_waitall_ws(cliente,sizeof(int));
	serialPagina = recv_waitall_ws(cliente,sizeof(int));
	int pid = char4ToInt(serialPID);
	int paginas = char4ToInt(serialPagina);
	asignarEspacioANuevoProceso(pid, paginas);
	// Hacer que asignarEspacioANuevoProceso devuelva el proceso asi evito buscarlo
	int posInicial = buscarProcesoSegunPID(pid)->posPagina;
	int i;
	log_info(activeLogger,"Se van a recibir %d paginas del pid %d",paginas,pid);
	for (i=0;i<paginas;i++){
		log_info(activeLogger,"Recibiendo pagina:%d",i);
		char* pagina = recv_waitall_ws(cliente,config.tamanio_pagina);
		escribirPagina(posInicial+i,pagina);
		free(pagina);
	}

	log_info(activeLogger,"Recibidas todas las paginas");

	enviarHeader(cliente,HeaderProcesoAgregado);
	free(serialPID);
	free(serialPagina);
	imprimirBitarray();
}
void operacionEscritura(){
	log_info(activeLogger, "Se recibio escritura");
	char* serialPID = malloc(sizeof(int));
	char* serialPagina = malloc(sizeof(int));
	char* contenido= malloc(config.tamanio_pagina);
	serialPID = recv_waitall_ws(cliente,sizeof(int));
	serialPagina = recv_waitall_ws(cliente,sizeof(int));
	int pid = char4ToInt(serialPID);
	int pagina = char4ToInt(serialPagina);
	contenido = recv_waitall_ws(cliente,config.tamanio_pagina);
	escribirPagina(buscarProcesoSegunPID(pid)->posPagina+pagina,contenido);
	enviarHeader(cliente,HeaderEscrituraCorrecta);
	//usleep(config.retardo_acceso);//TODO DESCOMENTAR PARA CUANDO SE PRUEBE EN SERIO
	free(serialPID);
	free(serialPagina);
	free(contenido);
}
void operacionLectura(){
	t_datosPedido* datosPedido = malloc(sizeof(t_datosPedido));
	log_info(activeLogger, "Se recibio lectura");
	char* serialPID = malloc(sizeof(int));
	char* serialPagina = malloc(sizeof(int));
	serialPID = recv_waitall_ws(cliente,sizeof(int));
	serialPagina = recv_waitall_ws(cliente,sizeof(int));
	int pid = char4ToInt(serialPID);
	int pagina = char4ToInt(serialPagina);
	char* contenido = leerPagina(
		buscarProcesoSegunPID(pid)->posPagina
			+ pagina);
	usleep(config.retardo_acceso);//TODO DESCOMENTAR PARA CUANDO SE PRUEBE EN SERIO
	enviarHeader(cliente, HeaderOperacionLectura);
	send_w(cliente, contenido, config.tamanio_pagina);
	free(contenido);


	free(serialPID);
	free(serialPagina);
	free(datosPedido);
}
void operacionFinalizar(){
	log_info(activeLogger, "Se recibio finalizacion");
	char* serialPID = malloc(sizeof(int));
	serialPID = recv_waitall_ws(cliente,sizeof(int));
	int pid = char4ToInt(serialPID);
    finalizarProceso(pid);
	enviarHeader(cliente,HeaderProcesoEliminado);

	free(serialPID);
}
// Funciones para el manejo del Espacio
int espaciosDisponibles(t_bitarray* unEspacio) {
	int i;
	int espacioSinUtilizar = 0;
	for (i = 0; i < config.cantidad_paginas; i++) {
		if (bitarray_test_bit(unEspacio, i) == 0)
			espacioSinUtilizar++;
	}
	return espacioSinUtilizar;
}
int espaciosUtilizados(t_bitarray* unEspacio) {
	int i;
	int espacioUtilizado = 0;
	for (i = 0; i < config.cantidad_paginas; i++) {
		if (bitarray_test_bit(unEspacio, i) == 1)
			espacioUtilizado++;
	}
	return espacioUtilizado;
}
void limpiarPosiciones(t_bitarray* unEspacio, int posicionInicial,
		int tamanioProceso) {
	int i = 0;
	for (i = posicionInicial; i < posicionInicial + tamanioProceso; i++) {
		if (bitarray_test_bit(espacio, i) == 1)
			espacioDisponible++;
		bitarray_clean_bit(unEspacio, i);
	}
}
void setearPosiciones(t_bitarray* unEspacio, int posicionInicial,
		int tamanioProceso) {
	int i = 0;
	for (i = posicionInicial; i < posicionInicial + tamanioProceso; i++)
		bitarray_set_bit(unEspacio, i);
	espacioDisponible -= tamanioProceso;
}
int hayQueCompactar(int paginasAIniciar) {
	return (buscarEspacio(paginasAIniciar) == -1);
}
int primerEspacioLibre() {
	int i;
	for (i = 0; i < config.cantidad_paginas; i++) {
		if (bitarray_test_bit(espacio, i) == 0)
			return i;
	}
	return 0;
}
void asignarEspacioANuevoProceso(int pid, int paginasAIniciar) {
	if (paginasAIniciar <= espacioDisponible) {
		//Me fijo si hay fragmentacion para asi ver si necesito compactar
		if (hayQueCompactar(paginasAIniciar)) {
			compactar();
//			if (hayQueCompactar(paginasAIniciar))
//				enviarHeader(cliente,HeaderNoHayEspacio);
		}
		agregarProceso(pid, paginasAIniciar);
	} else {
//		enviarHeader(cliente,HeaderNoHayEspacio);
		printf("No hay espacio suficiente para asignar al nuevo proceso.\n");
		log_error(activeLogger, "Fallo la iniciacion del programa %d ", pid);
	}
}
int buscarEspacio(int paginasAIniciar) {
	int i, espacioTotal = 0;
	for (i = 0; i < config.cantidad_paginas; i++) {
		if (bitarray_test_bit(espacio, i))
			espacioTotal =0;
		else {
			espacioTotal++;
			if (espacioTotal > paginasAIniciar) {
				return i - paginasAIniciar ;
			}
		}
	}
	if (++espacioTotal==paginasAIniciar+1)
		return config.cantidad_paginas-paginasAIniciar;

	return -1;
}
void compactar() {
	int i = 0;
	int espaciosLibres = 0;
	int nuevaPosicion;
	t_infoProceso* procesoActual;
	for (i = 0; i < config.cantidad_paginas; i++) {
		if (bitarray_test_bit(espacio, i) == 0)
			espaciosLibres++;
		if (espaciosLibres != 0 && bitarray_test_bit(espacio, i) == 1) {
			procesoActual = buscarProcesoSegunInicio(i);
			nuevaPosicion = (i - espaciosLibres);
			moverProceso(procesoActual, nuevaPosicion);

			i += (procesoActual->cantidadDePaginas) - 1;
		}
	}
	//usleep(config.retardo_compactacion); //TODO DESCOMENTAR PARA CUANDO SE PRUEBE EN SERIO
	log_info(activeLogger, "Compactación finalizada.");
}
void configurarBitarray() {
	/* bitarray manejo de paginas */
	int tamanio = (config.cantidad_paginas);
	char* data = malloc(tamanio + 1);
	strcpy(data, "\0");
	espacio = bitarray_create(data, tamanio);
	espacioUtilizado = list_create();
	//marco libres todos las posiciones del array
	limpiarPosiciones(espacio, 0, config.cantidad_paginas);
}
void imprimirBitarray() {
	int i;
	for (i = 0; i < config.cantidad_paginas; i++) {
		printf("%d", bitarray_test_bit(espacio, i));
	}
	printf("\n");
}
void limpiarEstructuras() {
	limpiarPosiciones(espacio, 0, config.cantidad_paginas);
	bzero(archivo, config.cantidad_paginas * config.tamanio_pagina);
	//list_clean(espacioUtilizado);
	list_clean_and_destroy_elements(espacioUtilizado,free);
	espacioDisponible = config.cantidad_paginas;
}
// Funciones para el manejo de Paginas
void escribirPagina(int numeroPagina, char* contenidoPagina) {
	log_info(activeLogger, "Se escribio la pagina:%d el contenido:%s",numeroPagina,contenidoPagina);
	memcpy(archivo + numeroPagina * config.tamanio_pagina, contenidoPagina, config.tamanio_pagina);
}
char* leerPagina(int numeroPagina) {
	log_info(activeLogger, "Se leyo la pagina:%d",numeroPagina);
	char* str = malloc(config.tamanio_pagina);
	memcpy(str, archivo + numeroPagina * config.tamanio_pagina,
			config.tamanio_pagina);
	return str;
}
void imprimirPagina(int numeroPagina) {
	char* str = malloc(config.tamanio_pagina + 1);
	memcpy(str, archivo + numeroPagina * config.tamanio_pagina,
			config.tamanio_pagina);
	str[config.tamanio_pagina] = '\0';
	printf("%s\n", str);
}
void moverPagina(int numeroPagina, int posicion) {
	log_info(activeLogger, "Se movio la pagina:%d a la posicion:%d",numeroPagina,posicion);
	memcpy(archivo + posicion * config.tamanio_pagina,
			archivo + numeroPagina * config.tamanio_pagina,
			config.tamanio_pagina);
	bzero(archivo + numeroPagina * config.tamanio_pagina,
			config.tamanio_pagina);
}
// Funciones para el manejo de Procesos
void agregarProceso(int pid, int paginasAIniciar) {
	//  en asignarEspacioANuevoProceso se chequeo que va a haber espacio si o si
	int marcoInicial = buscarEspacio(paginasAIniciar);
	if (marcoInicial >= 0) {
		setearPosiciones(espacio, marcoInicial, paginasAIniciar);
		//Definimos la estructura del nuevo proceso con los datos correspondientes y lo agregamos al espacio utilizado
		t_infoProceso* proceso = (t_infoProceso*) malloc(sizeof(t_infoProceso));
		proceso->pid = pid;
		proceso->posPagina = marcoInicial;
		proceso->cantidadDePaginas = paginasAIniciar;
		list_add(espacioUtilizado, (void*) proceso);
		log_info(activeLogger, "Se inicializo el proceso pid:%d",pid);
		//printf("Proceso agregado exitosamente\n");
//		enviarHeader(cliente,HeaderProcesoAgregado);
	} else {
//		enviarHeader(cliente, HeaderError);
		printf("Error Nunca debio llegar acá al agregar Proceso\n");
	}
}
void moverProceso(t_infoProceso* proceso, int nuevoInicio) {
	limpiarPosiciones(espacio, proceso->posPagina, proceso->cantidadDePaginas);
	int i;
	for (i = 0; i < proceso->cantidadDePaginas; i++) {
		moverPagina(proceso->posPagina + i, nuevoInicio + i);
	}
	setearPosiciones(espacio, nuevoInicio, proceso->cantidadDePaginas);
	proceso->posPagina = nuevoInicio;
}
void finalizarProceso(int pid) {
	//Busco el proceso en la lista de espacio utilizado y lo elimino de la lista
	t_infoProceso* proceso;
	int i = 0;
	int cantidadProcesos = list_size(espacioUtilizado);
	for (i = 0; i < cantidadProcesos; i++) {
		proceso = (t_infoProceso*) list_get(espacioUtilizado, i);
		if (proceso->pid == pid) {
			limpiarPosiciones(espacio, proceso->posPagina,
					proceso->cantidadDePaginas);
			list_remove_and_destroy_element(espacioUtilizado, i, free);
			log_info(activeLogger,
					"El programa %d - Pagina Inicial:%d Tamanio:%d Eliminado correctamente.",
					pid, proceso->posPagina, proceso->cantidadDePaginas);
			break;
		}
	}
}
bool estaProceso(int unPid) {
	int i;
	int cantidadProcesos = list_size(espacioUtilizado);
	t_infoProceso* datoProceso = malloc(sizeof(t_infoProceso));
	for (i = 0; i < cantidadProcesos; i++) {
		datoProceso = (t_infoProceso*) list_get(espacioUtilizado, i);
		if (datoProceso->pid == unPid)
			return true;
	}
	return false;
}
t_infoProceso* buscarProcesoSegunPID(int unPid) {
	int i;
	int cantidadProcesos = list_size(espacioUtilizado);
	t_infoProceso* datoProceso = malloc(sizeof(t_infoProceso));
	for (i = 0; i < cantidadProcesos; i++) {
		datoProceso = (t_infoProceso*) list_get(espacioUtilizado, i);
		if (datoProceso->pid == unPid)
			return datoProceso;
	}
	return (t_infoProceso*) 0;
}
t_infoProceso* buscarProcesoSegunInicio(int marcoInicial) {
	int i;
	printf("BUSCANDO PROCESO POR INICIO\n");
	int cantidadProcesos = list_size(espacioUtilizado);
	t_infoProceso* datoProceso; //= malloc(sizeof(t_infoProceso));
	for (i = 0; i < cantidadProcesos; i++) {
		datoProceso = (t_infoProceso*) list_get(espacioUtilizado, i);
		if (datoProceso->posPagina == marcoInicial)
			return datoProceso;
	}
	return NULL;
}

int existeElPid(int unPid){
	int i;
	int cantidadProcesos = list_size(espacioUtilizado);
	t_infoProceso* datoProceso = malloc(sizeof(t_infoProceso));
	int estado=0;
	for (i = 0; i < cantidadProcesos; i++) {
		datoProceso = (t_infoProceso*) list_get(espacioUtilizado, i);
		if (datoProceso->pid == unPid) estado=1;
	}
	return estado;
}


//**************************************************MAIN SWAP*****************************************************************
int main() {
	inicializar();
	testear(test_swap);
	conectar_umc();
	esperar_peticiones();
	finalizar();
	return 0;
}

