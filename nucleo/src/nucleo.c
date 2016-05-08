/*
 * nucleo.c
 *
 *  Created on: 16/4/2016
 *      Author: utnso
 */

#include "nucleo.h"

/*  ----------INICIO PARA PLANIFICACION ---------- */
bool pedirPaginas(int PID, char* codigo) {
	int hayMemDisponible;
	char respuesta;
	if (DEBUG_IGNORE_UMC || DEBUG_IGNORE_UMC_PAGES) { // Para DEBUG
		log_warning(activeLogger,
				"DEBUG_IGNORE_UMC_PAGES está en true! Se supone que no hay paginas");
		hayMemDisponible = false;
	} else {    // Para curso normal del programa
		//Estos mutex garantizan que por ejemplo no haga cada hilo un send (esto podria solucionarse tambien juntando los send, pero es innecesario porque si o si hay que sincronizar)
		//y que no se van a correr los sends de un hilo 1, los del hilo 2, umc responde por hilo 1 (lo primero que le llego) y como corre hilo 2, esa respuesta llega al hilo 2 en vez de al 1.
		pthread_mutex_lock(&lock_UMC_conection);
		send_w(umc, headerToMSG(HeaderScript), 1);
		send_w(umc, intToChar(strlen(codigo)), 1); //fixme: un char admite de 0 a 255. SI el tamaño supera eso se rompe!
		send_w(umc, codigo, strlen(codigo));
		read(umc, &respuesta, 1);
		pthread_mutex_unlock(&lock_UMC_conection);
		hayMemDisponible = (int) respuesta;
		if (hayMemDisponible != 0 && hayMemDisponible != 1) {
			log_warning(activeLogger,
					"Umc debería enviar un booleano (0 o 1) y envió %d",
					hayMemDisponible);
		}
		log_debug(bgLogger, "Hay memoria disponible para el proceso %d.", PID);;
	}
	return (bool) hayMemDisponible;
}

char* getScript(int consola) {
	char scriptSize;
	char* script;
	int size;
	read(clientes[consola].socket, &scriptSize, 1);
	size = charToInt(&scriptSize);
	log_debug(bgLogger, "Consola envió un archivo de tamaño: %d", size);
	printf("Size:%d\n", size);
	script = malloc(sizeof(char) * size);
	read(clientes[consola].socket, script, size);
	log_info(activeLogger, "Script:\n%s", script);
	clientes[consola].atentido=false; //En true se bloquean, incluso si mando muchos de una consola usando un FOR para mandar el comando (leer wikia)
	return script;
}

void rechazarProceso(int PID) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_remove(listaProcesos, PID);
	pthread_mutex_unlock(&lockProccessList);
	if (proceso->estado != NEW)
		log_warning(activeLogger,
				"Se esta rechazando el proceso %d ya aceptado!", PID);
	send(clientes[proceso->consola].socket,
			intToChar(HeaderConsolaFinalizarRechazado), 1, 0); // Le decimos adios a la consola
	log_info(bgLogger,"Consola avisada sobre la finalización del proceso ansisop.");
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	free(proceso); // Destruir Proceso y PCB
}

int crearProceso(int consola) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = malloc(sizeof(t_proceso));
	proceso->PCB.PID = list_add(listaProcesos, proceso);
	pthread_mutex_unlock(&lockProccessList);
	proceso->PCB.PC = SIN_ASIGNAR;
	proceso->PCB.SP = (t_stack*) SIN_ASIGNAR;
	proceso->estado = NEW;
	proceso->consola = consola;
	proceso->cpu = SIN_ASIGNAR;
	char* codigo = getScript(consola);
	// Si la UMC me rechaza la solicitud de paginas, rechazo el proceso
	if (!pedirPaginas(proceso->PCB.PID, codigo)) {
		printf("rechazado");
		rechazarProceso(proceso->PCB.PID);
		log_info(activeLogger, "UMC no da paginas para el proceso %d!",
				proceso->PCB.PID);
		log_info(activeLogger, "Se rechazo el proceso %d.", proceso->PCB.PID);
	}

	free(codigo);
	return proceso->PCB.PID;
}

void cargarProceso(int consola) {
	// Crea un hilo que crea el proceso y se banca esperar a que umc le de paginas.
	// Mientras tanto, el planificador sigue andando.
	pthread_create(&crearProcesos, &detachedAttr, (void*) crearProceso,
			(void*) consola);
}

void ejecutarProceso(int PID, int cpu) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_get(listaProcesos, PID);
	pthread_mutex_unlock(&lockProccessList);
	if (proceso->estado != READY)
		log_warning(activeLogger, "Ejecucion del proceso %d sin estar listo!",
				PID);
	proceso->estado = EXEC;
	proceso->cpu = cpu;
	// todo: mandarProcesoCpu(cpu, proceso->PCB);
}

void finalizarProceso(int PID) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_get(listaProcesos, PID);
	pthread_mutex_unlock(&lockProccessList);
	queue_push(colaCPU, (int*) proceso->cpu); // Disponemos de nuevo de la CPU
	proceso->cpu = SIN_ASIGNAR;
	proceso->estado = EXIT;
	queue_push(colaSalida, (void*) PID);
}

void destruirProceso(int PID) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_remove(listaProcesos, PID);
	pthread_mutex_unlock(&lockProccessList);
	if (proceso->estado != EXIT)
		log_warning(activeLogger,
				"Se esta destruyendo el proceso %d que no libero sus recursos!",
				PID);
	send(clientes[proceso->consola].socket,
			intToChar(HeaderConsolaFinalizarNormalmente), 1, 0); // Le decimos adios a la consola
	quitarCliente(proceso->consola); // Esto no es necesario, ya que si la consola funciona bien se desconectaria, pero quien sabe...
	// todo: avisarUmcQueLibereRecursos(proceso->PCB) // e vo' umc liberá los datos
	free(proceso); // Destruir Proceso y PCB
}

void actualizarPCB(t_PCB PCB){ //
	// Cuando CPU me actualice la PCB del proceso me manda una PCB (no un puntero)
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_get(listaProcesos, PCB.PID);
	pthread_mutex_unlock(&lockProccessList);
	proceso->PCB=PCB;
}

bool terminoQuantum(t_proceso* proceso){
	return (!(proceso->PCB.PC%config.quantum)); // Si el PC es divisible por QUANTUM quiere decir que hizo QUANTUM ciclos
}

void expulsarProceso(t_proceso* proceso){
	if (proceso->estado!=EXEC)
		log_warning(activeLogger, "Expulsion del proceso %d sin estar ejecutandose!",proceso->PCB.PID);
	proceso->estado=READY;
	queue_push(colaListos, (void*) proceso->PCB.PID);
	queue_push(colaCPU, (void*) proceso->cpu); // Disponemos de la CPU
	proceso->cpu = SIN_ASIGNAR;
}

void planificarProcesos() {
	int i;
	switch (algoritmo) {
	// Procesos especificos
	case RR:

		for(i=0;i<list_size(listaProcesos);i++){
			pthread_mutex_lock(&lockProccessList);
			t_proceso* proceso = list_get(listaProcesos,i);
			pthread_mutex_unlock(&lockProccessList);
			if (proceso->estado==EXEC){
				if (terminoQuantum(proceso))
					expulsarProceso(proceso);
			}
		}
		/*NOTA: RR usa FIFO, así que sin break*/
	case FIFO:

		if (!queue_is_empty(colaListos) && !queue_is_empty(colaCPU))
			ejecutarProceso((int) queue_pop(colaListos), (int) queue_pop(colaCPU));

		if (!queue_is_empty(colaSalida))
			destruirProceso((int) queue_pop(colaSalida));

		break;
	}
	//printf("planificando...\n");
}

void bloquearProceso(int PID, int IO) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_get(listaProcesos, PID);
	pthread_mutex_unlock(&lockProccessList);
	if (proceso->estado != EXEC)
		log_warning(activeLogger,
				"El proceso %d se bloqueo pese a que no estaba ejecutando!",
				PID);
	proceso->estado = BLOCK;
	queue_push(colaCPU, (void*) proceso->cpu); // Disponemos de la CPU
	proceso->cpu = SIN_ASIGNAR;
	// todo: Añadir a la cola de ese IO
}

void desbloquearProceso(int PID) {
	pthread_mutex_lock(&lockProccessList);
	t_proceso* proceso = list_get(listaProcesos, PID);
	pthread_mutex_unlock(&lockProccessList);
	if (proceso->estado != BLOCK)
		log_warning(activeLogger,
				"Desbloqueando el proceso %d sin estar bloqueado!", PID);
	proceso->estado = READY;
	queue_push(colaListos, (void*) PID);
}
/* ---------- FIN PARA PLANIFICACION ---------- */



// Funcion para obtener los Int de los array de configuracion
int AsciiToInt(char* var){
	return atoi(var);
}

void cargarCFG() {
	t_config* configNucleo;
	configNucleo = config_create("nucleo.cfg");
	config.puertoConsola = config_get_int_value(configNucleo, "PUERTO_PROG");
	config.puertoCPU = config_get_int_value(configNucleo, "PUERTO_CPU");
	config.quantum = config_get_int_value(configNucleo, "QUANTUM");
	config.queantum_sleep = config_get_int_value(configNucleo, "QUANTUM_SLEEP");
	config.sem_ids =	config_get_array_value(configNucleo, "SEM_ID");
	//retorna chars, no int, pero como internamente son lo mismo, entender un puntero como a char* o a int* es indistinto
	config.semInit = config_get_array_value(configNucleo, "SEM_INIT");
	config.io_ids = config_get_array_value(configNucleo, "IO_ID");
	//retorna chars, no int, pero como internamente son lo mismo, entender un puntero como a char* o a int* es indistinto
	config.ioSleep = config_get_array_value(configNucleo, "IO_SLEEP");
	config.sharedVars = config_get_array_value(configNucleo, "SHARED_VARS");
	config.ipUMC = config_get_string_value(configNucleo, "IP_UMC");
	config.puertoUMC = config_get_int_value(configNucleo, "PUERTO_UMC");
	/*
		Ejemplo de array 'int'
	printf("SEM_ID[2]=%d\n",AsciiToInt(config.semInit[2]));
		Ejemplo de array de Strings
	printf("IO_ID[2]=%s\n",config.io_ids[2]);

	*/
}

void configHilos(){
	pthread_attr_init(&detachedAttr);
	pthread_attr_setdetachstate(&detachedAttr,PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&lockProccessList,NULL);
	pthread_mutex_init(&lock_UMC_conection,NULL);
}

int getConsolaAsociada(int cliente) {
	int PID = charToInt(recv_waitall_ws(cliente, sizeof(int)));
	t_proceso* proceso = list_get(listaProcesos, PID);
	return proceso->consola;
}

void imprimirVariable(int cliente) {
	int consola = getConsolaAsociada(cliente);
	char* msgValue = recv_waitall_ws(cliente, sizeof(ansisop_var_t));
	char* name = recv_waitall_ws(cliente, sizeof(char));
	send_w(consola, headerToMSG(HeaderImprimirVariableConsola), 1);
	send_w(consola, msgValue, sizeof(ansisop_var_t));
	send_w(consola, name, sizeof(char));
	free(msgValue);
	free(name);
}

void imprimirTexto(int cliente) {
	int consola = getConsolaAsociada(cliente);
	char* msgSize = recv_waitall_ws(cliente, sizeof(int));
	int size = charToInt(msgSize);
	char* texto = recv_waitall_ws(cliente, size);
	send_w(consola, headerToMSG(HeaderImprimirTextoConsola), 1);
	send_w(consola, intToChar(size), 1);
	send_w(consola, texto, size);
	free(msgSize);
	free(texto);
}

void procesarHeader(int cliente, char *header) {
	// Segun el protocolo procesamos el header del mensaje recibido
	char* payload;
	int payload_size;
	log_debug(bgLogger, "Llego un mensaje con header %d", charToInt(header));
	clientes[cliente].atentido=true;

	switch (charToInt(header)) {

	case HeaderError:
		log_error(activeLogger, "Header de Error");
		quitarCliente(cliente);
		break;

	case HeaderHandshake:
		log_debug(bgLogger, "Llego un handshake");
		payload_size = 1;
		payload = malloc(payload_size);
		read(clientes[cliente].socket, payload, payload_size);
		log_debug(bgLogger, "Llego un mensaje con payload %d",
				charToInt(payload));
		if ((charToInt(payload) == SOYCONSOLA)
				|| (charToInt(payload) == SOYCPU)) {
			log_debug(bgLogger,
					"Es un cliente apropiado! Respondiendo handshake");
			clientes[cliente].identidad = charToInt(payload);
			send(clientes[cliente].socket, intToChar(SOYNUCLEO), 1, 0);
		} else {
			log_error(activeLogger,
					"No es un cliente apropiado! rechazada la conexion");
			log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
			quitarCliente(cliente);
		}
		free(payload);
		clientes[cliente].atentido=false;
		break;

	case HeaderImprimirVariableNucleo:
		imprimirVariable(cliente);
		break;

	case HeaderImprimirTextoNucleo:
		imprimirTexto(cliente);
		break;

	case HeaderScript:
		cargarProceso(cliente);
		break;

	default:
		log_error(activeLogger, "Llego cualquier cosa.");
		log_error(activeLogger,
				"Llego el header numero %d y no hay una acción definida para él.",
				charToInt(header));
		log_warning(activeLogger, "Se quitará al cliente %d.", cliente);
		quitarCliente(cliente);
		break;
	}
}

struct timeval newEspera() {
	struct timeval espera;
	espera.tv_sec = 2; 				//Segundos
	espera.tv_usec = 500000; 		//Microsegundos
	return espera;
}



/* ---------- INICIO PARA UMC ---------- */
// Dejo toodo sin espacios en el medio cuestion de que al ver solo las firmas
// de las funciones, esta parte se saltee rapido. Idealmente,
// no habria que tocarla mas :)
int getHandshake() {
	char* handshake = recv_nowait_ws(umc, 1);
	return charToInt(handshake);
}
void handshakearUMC() {
	char *hand = string_from_format("%c%c", HeaderHandshake, SOYNUCLEO);
	send_w(umc, hand, 2);

	log_debug(bgLogger, "UMC handshakeo.");
	if (getHandshake() != SOYUMC) {
		perror("Se esperaba conectarse a la UMC.");
	} else
		log_debug(bgLogger, "Núcleo recibió handshake de UMC.");
}
void establecerConexionConUMC() {
	direccionParaUMC = crearDireccionParaCliente(config.puertoUMC);
	umc = socket_w();
	connect_w(umc, &direccionParaUMC);
}
void warnDebug() {
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---");
	log_info(activeLogger, "NO SE ESTABLECE CONEXION CON UMC EN ESTE MODO!");
	log_info(activeLogger,
			"Para correr nucleo en modo normal, settear en false el define DEBUG_IGNORE_UMC.");
	log_warning(activeLogger, "--- CORRIENDO EN MODO DEBUG!!! ---");
}
void conectarAUMC() {
	if (!DEBUG_IGNORE_UMC) {
		log_debug(bgLogger, "Iniciando conexion con UMC...");
		establecerConexionConUMC();
		log_info(activeLogger, "Conexion a la UMC correcta :).");
		handshakearUMC();
		log_info(activeLogger, "Handshake con UMC finalizado exitosamente.");
	} else {
		warnDebug();
	}
}
/* ---------- FIN PARA UMC ---------- */


void finalizar() {
	destruirLogs();
	list_destroy(listaProcesos);
	queue_destroy(colaCPU);
	queue_destroy(colaListos);
	queue_destroy(colaSalida);
	pthread_mutex_destroy(&lockProccessList);
	pthread_mutex_destroy(&lock_UMC_conection);
	pthread_attr_destroy(&detachedAttr);
}

int main(void) {
	system("clear");
	int i;
	struct timeval espera = newEspera(); // Periodo maximo de espera del select
	char header[1];
	listaProcesos = list_create();
	colaCPU = queue_create();
	colaListos = queue_create();
	colaSalida = queue_create();
	pthread_mutex_init(&lockProccessList, NULL);

	cargarCFG();
	configHilos();

	crearLogs("Nucleo", "Nucleo");

	configurarServidorExtendido(&socketConsola, &direccionConsola,
			config.puertoConsola, &tamanioDireccionConsola, &activadoConsola);
	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puertoCPU,
			&tamanioDireccionCPU, &activadoCPU);

	inicializarClientes();
	log_info(activeLogger, "Esperando conexiones ...");


	conectarAUMC();


	while (1) {
		FD_ZERO(&socketsParaLectura);
		FD_SET(socketConsola, &socketsParaLectura);
		FD_SET(socketCPU, &socketsParaLectura);

		incorporarClientes();
		if ((socketConsola > mayorDescriptor)
				|| (socketCPU > mayorDescriptor)) {
			if (socketConsola > socketCPU)
				mayorDescriptor = socketConsola;
			else
				mayorDescriptor = socketCPU;
		}

		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, &espera);

		if (tieneLectura(socketConsola))
			procesarNuevasConexionesExtendido(&socketConsola);

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

		//planificarProcesos();
	}

	finalizar();
	return EXIT_SUCCESS;
}
