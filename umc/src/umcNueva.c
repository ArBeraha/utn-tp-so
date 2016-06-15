///*
// * umcNueva.c
// *
// *  Created on: 11/6/2016
// *      Author: utnso
// */
//#include "umc.h"
//
//// HILO PRINCIPAL: main()
//// FUNCIONES: procesar las nuevas conexiones y crearles un hilo propio
//HILO main2() {
//	crearLogs("Umc","Proceso",0);
//	log_info(activeLogger, "Iniciando umcNueva");
//	cargarCFG();
//	iniciarAtrrYMutexs(1,&mutexClientes);
//	log_info(activeLogger, "Conectando a SWAP ...");
//	conectarASwap();
//
//	configurarServidorExtendido(&socketNucleo, &direccionNucleo, config.puerto_umc_nucleo,
//					&tamanioDireccionNucleo, &activadoNucleo);
//
//	configurarServidorExtendido(&socketCPU, &direccionCPU, config.puerto_cpu,
//			&tamanioDireccionCPU, &activadoCPU);
//
//	inicializarClientes();
//	log_info(activeLogger, "Esperando conexiones ...");
//
//	while (1) {
//		FD_ZERO(&socketsParaLectura);
//		FD_SET(socketNucleo, &socketsParaLectura);
//		FD_SET(socketCPU, &socketsParaLectura);
//
//		mayorDescriptor = (socketNucleo > socketCPU) ? socketNucleo : socketCPU;
//
//		select(mayorDescriptor + 1, &socketsParaLectura, NULL, NULL, NULL);
//
//		int cliente;
//		if (tieneLectura(socketCPU))  {
//			log_info(activeLogger, "Se conecto una nueva CPU");
//			if ((cliente = procesarNuevasConexionesExtendido(&socketCPU))>=0)
//			crearHiloConParametro(&clientes[cliente].hilo,(HILO)hiloDedicado,(void*)cliente);
//		}
//		if (tieneLectura(socketNucleo)) {
//			log_info(activeLogger, "Se conecto Nucleo");
//			if ((cliente = procesarNuevasConexionesExtendido(&socketNucleo))>=0)
//			crearHiloConParametro(&clientes[cliente].hilo,(HILO)hiloDedicado,(void*)cliente);
//		}
//	}
//}
//
//
//void llamarSwap(t_cliente cliente){
//	char* serialPID = intToChar4(cliente.indice);
//	char* serialCantidadPaginas = intToChar4(5);
//	char* serialPagina = intToChar4(2);
//	char* contenidoPagina = malloc(config.tamanio_marco);
//	memcpy(contenidoPagina,"abcdefg",7);
//	bzero(contenidoPagina+7,config.tamanio_marco-7);
//
//	// INICIAR PROCESO
//	enviarHeader(swapServer,HeaderOperacionIniciarProceso);
//	send_w(swapServer,serialPID,sizeof(int));
//	send_w(swapServer,serialCantidadPaginas,sizeof(int));
//	char* header = recv_waitall_ws(swapServer,1);
//	if (charToInt(header)==HeaderProcesoAgregado)
//		printf("Contesto el proceso Agregado\n");
//	else
//	if (charToInt(header)==HeaderNoHayEspacio){
//		printf("No hay espacio\n");
//		return;}
//	else{
//		printf("Llego mierda %d\n",(int)header);return;}
//
//	// ESCRIBIR PAGINA
//	enviarHeader(swapServer,HeaderOperacionEscritura);
//	send_w(swapServer,serialPID,sizeof(int));
//	send_w(swapServer,serialPagina,sizeof(int));
//	send_w(swapServer,contenidoPagina,config.tamanio_marco);
//	header = recv_waitall_ws(swapServer,1);
//		if (charToInt(header)==HeaderEscrituraCorrecta)
//			log_info(activeLogger,"Escritura correcta");
//		else if (charToInt(header)==HeaderEscrituraErronea)
//			log_warning(activeLogger,"Escritura erronea");
//		else log_error(activeLogger,"Llego mierda al escribir");
//
//
//	// LEER PAGINA
//	enviarHeader(swapServer,HeaderOperacionLectura);
//	char* contenidoPagina2 = malloc(config.tamanio_marco+1);
//	send_w(swapServer,serialPID,sizeof(int));
//	send_w(swapServer,serialPagina,sizeof(int));
//
//	header = recv_waitall_ws(swapServer,1);
//	if (charToInt(header)==HeaderOperacionLectura)
//		log_info(activeLogger,"Contesto con la pagina");
//	else if (charToInt(header)==HeaderProcesoNoEncontrado)
//		log_warning(activeLogger,"No la encontró");
//	else log_error(activeLogger,"Llego mierda al leer");
//
//	contenidoPagina2 = recv_waitall_ws(swapServer,config.tamanio_marco);
//	contenidoPagina2[config.tamanio_marco]='\0';
//	log_info(activeLogger,"Llego el msg:%s",contenidoPagina2);
////	log_info(activeLogger,"Llego el contenido y es igual:%d\n",strcmp(contenidoPagina,contenidoPagina2)==0);
//
//	// FINALIZAR PROCESO
//	enviarHeader(swapServer,HeaderOperacionFinalizarProceso);
//	send_w(swapServer,serialPID,sizeof(int));
//
//	header = recv_waitall_ws(swapServer,1);
//	if (charToInt(header)==HeaderProcesoEliminado)
//		log_info(activeLogger,"Se elimino bien");
//	else if (charToInt(header)==HeaderProcesoNoEncontrado)
//		log_warning(activeLogger,"Se elimino mal");
//	else log_error(activeLogger,"Llego mierda al leer");
//
//	free(serialPID);
//	free(header);
//	free(serialCantidadPaginas);
//	free(serialPagina);
//	free(contenidoPagina);
//	free(contenidoPagina2);
//}
//
//
//// HILO HIJO: cpu()
//// FUNCION: Atender los headers de las cpus
//HILO hiloDedicado(int indice) {
//	log_info(activeLogger, "Se creó un hilo dedicado");
//	t_cliente clienteLocal; // Generamos una copia del cliente, no sirve para datos actualizables como el pid, solo para permanentes como socket e indice, para los demas campos consultar con mutex el vector de clientes
//	MUTEXCLIENTES(clienteLocal = clientes[indice]);
//	printf("SOCKET:%d\n",clienteLocal.socket);
//	printf("INDICE:%d\n",clienteLocal.indice);
//	char* header = malloc(1);
//	while (recv(clienteLocal.socket, header, 1, MSG_WAITALL)>=0){
//		procesarHeader2(clienteLocal, header);
//	}
//	free(header);
//	log_info(activeLogger, "Hasta aqui llego el hilo");
//	return 0;
//}



