/*
 * test.c
 *
 *  Created on: 4/6/2016
 *      Author: utnso
 */
#include "swap.h"

/****************************************TEST***************************************************************/
void testSwapDeBitArray1() //No hay espacio para el proceso, hay que compactar
{

 printf("******************testSwapDeBitArray1 ha comenzado***********************\n");
 bitarray_clean_bit(espacio, 0);
 bitarray_set_bit(espacio, 1);
 bitarray_set_bit(espacio, 2);
 bitarray_clean_bit(espacio, 3);
 bitarray_clean_bit(espacio, 4);
 bitarray_set_bit(espacio, 5);
 bitarray_clean_bit(espacio, 6);
 bitarray_clean_bit(espacio, 7);
 bitarray_set_bit(espacio, 8);
 setearPosiciones (espacio,9,config.cantidad_paginas);

 if(hayQueCompactar(3)) printf("Test de posibilidad de compactacion superado\n");
 else printf("Test de posibilidad de compactacion no fue superado\n");
 espacioDisponible = config.cantidad_paginas;
 limpiarPosiciones (espacio,0,config.cantidad_paginas);
 list_clean(espacioUtilizado);
}

void testSwapDeBitArray2() // HAy espacio para el proceso, no hay que compactar
{
	printf("******************testSwapDeBitArray2 ha comenzado***********************\n");
bitarray_clean_bit(espacio, 0);
bitarray_set_bit(espacio, 1);
bitarray_clean_bit(espacio, 2);
bitarray_clean_bit(espacio, 3);
bitarray_clean_bit(espacio, 4);
bitarray_set_bit(espacio, 5);
bitarray_clean_bit(espacio, 6);
bitarray_clean_bit(espacio, 7);
bitarray_set_bit(espacio, 8);
 setearPosiciones (espacio,9,config.cantidad_paginas);

 if(hayQueCompactar(3)) printf("Test de posibilidad de compactacion no fue superado\n");
 else printf("Test de posibilidad de compactacion fue superado\n");

 espacioDisponible = config.cantidad_paginas;
 limpiarPosiciones (espacio,0,config.cantidad_paginas);
 list_clean(espacioUtilizado);
}

void testSwapDeCompactacion3() //TODO Y TAMBIEN TEST DE AGREGAR PROCESO E INICIAR PROCESO PROBAAAAAAAR
{
	int algo=0;
	int i=0;
  printf("******************testSwapDeCompactacion3 ha comenzado***********************\n");
  bitarray_set_bit(espacio, 0);
  bitarray_set_bit(espacio, 1);
  bitarray_set_bit(espacio, 2);
  bitarray_clean_bit(espacio, 3);
  bitarray_clean_bit(espacio, 4);
  bitarray_set_bit(espacio, 5);
  bitarray_set_bit(espacio, 6);
  bitarray_set_bit(espacio, 7);
  bitarray_clean_bit(espacio, 8);
  setearPosiciones (espacio,9,config.cantidad_paginas);

  t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
  	proceso1->pid = 5;
  	proceso1->posPagina = 0;
  	proceso1->cantidadDePaginas = 3;
  	list_add(espacioUtilizado,(void*) proceso1);

  t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
  	proceso2->pid = 8;
  	proceso2->posPagina = 5;
  	proceso2->cantidadDePaginas = 3;
  	list_add(espacioUtilizado,(void*) proceso2);

  	t_infoProceso* proceso3 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
  	  	proceso3->pid = 9;
  	  	proceso3->posPagina = 9;
  	    proceso3->cantidadDePaginas = config.cantidad_paginas-proceso3->posPagina;
  	  	list_add(espacioUtilizado,(void*) proceso3);


  	t_infoProceso* procesoAImprimir=malloc(sizeof(t_infoProceso));
  	procesoAImprimir=buscarProcesoAPartirDeMarcoInicial(0);
  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
  			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);


  	t_infoProceso*  procesoAImprimir2=malloc(sizeof(t_infoProceso));
  	procesoAImprimir2=buscarProcesoAPartirDeMarcoInicial(5);
  	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
  	  		 procesoAImprimir2->posPagina, procesoAImprimir2->pid, procesoAImprimir2->cantidadDePaginas);

  	t_infoProceso*  procesoAImprimir3=malloc(sizeof(t_infoProceso));
  	  	procesoAImprimir3=buscarProcesoAPartirDeMarcoInicial(12);
  	  	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
  	  	  		 procesoAImprimir3->posPagina, procesoAImprimir3->pid, procesoAImprimir3->cantidadDePaginas);

  compactar();

//  if(hayQueCompactar(3)) printf("Test de posibilidad de compactacion no fue superado\n");
//  else printf("Test de posibilidad de compactacion fue superado\n");
  algo= primerEspacioLibre(espacio);
  printf("La primera pagina libre es %d \n", algo);
  for(i=0;i<config.cantidad_paginas;i++)
  {
	  printf("En la posicion %d tengo el bit %d \n", i, bitarray_test_bit(espacio, i) );
  }


    	procesoAImprimir=buscarProceso(5);
    	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
    			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);



    	procesoAImprimir2=buscarProceso(8);
    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
    	  		 procesoAImprimir2->posPagina, procesoAImprimir2->pid, procesoAImprimir2->cantidadDePaginas);

    	procesoAImprimir3=buscarProceso(12);
    	    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
    	    	  		 procesoAImprimir3->posPagina, procesoAImprimir3->pid, procesoAImprimir3->cantidadDePaginas);



  espacioDisponible = config.cantidad_paginas;
  limpiarPosiciones (espacio,0,config.cantidad_paginas);
  list_clean(espacioUtilizado);

}

void testSwapDeCompactacion4()
{

	int i=0;
    printf("******************testSwapDeCompactacion3 ha comenzado***********************\n");
    limpiarPosiciones (espacio,0,config.cantidad_paginas-1);
    bitarray_set_bit(espacio, config.cantidad_paginas-1);

    for(i=0;i<config.cantidad_paginas;i++)
      	  {
      		  printf("En la posicion %d tengo el bit %d \n", i, bitarray_test_bit(espacio, i) );
      	  }

  t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
  	proceso1->pid = 5;
  	proceso1->posPagina = config.cantidad_paginas-1;
  	proceso1->cantidadDePaginas = 1;
  	list_add(espacioUtilizado,(void*) proceso1);

  	t_infoProceso* procesoAImprimir=malloc(sizeof(t_infoProceso));
  	  	procesoAImprimir=buscarProcesoAPartirDeMarcoInicial(config.cantidad_paginas-1);
  	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
  	  			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);

  	  compactar();

  	for(i=0;i<config.cantidad_paginas;i++)
  	  {
  		  printf("En la posicion %d tengo el bit %d \n", i, bitarray_test_bit(espacio, i) );
  	  }


  	    	procesoAImprimir=buscarProceso(5);
  	    	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
  	    			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);

  	    	espacioDisponible = config.cantidad_paginas;
  	    	  limpiarPosiciones (espacio,0,config.cantidad_paginas);
  	    	  list_clean(espacioUtilizado);
}

void testFinalizarProceso1() //esta el proceso, se elimina. No se usa puramente finalizarProceso
{
	printf("******************testFinalizarProceso1 ha comenzado***********************\n");
	t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso1->pid = 5;
	proceso1->posPagina = 0;
	proceso1->cantidadDePaginas = 3;
	list_add(espacioUtilizado,(void*) proceso1);

	t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso2->pid = 8;
	proceso2->posPagina = 8;
	proceso2->cantidadDePaginas = 6;
	list_add(espacioUtilizado,(void*) proceso2);

	bitarray_set_bit(espacio, 0);
	bitarray_set_bit(espacio, 1);
	bitarray_set_bit(espacio, 2);
	bitarray_clean_bit(espacio, 3);
	bitarray_clean_bit(espacio, 4);
	bitarray_clean_bit(espacio, 5);
	bitarray_clean_bit(espacio, 6);
	bitarray_clean_bit(espacio, 7);
	bitarray_clean_bit(espacio, 8);
	setearPosiciones (espacio,9,config.cantidad_paginas);

	//finalizarProceso(5); UTILIZO UNA VERSION SIN SOCKETS NI LOGS

		t_infoProceso* proceso;
		if (estaElProceso(5))
		{
		 proceso = buscarProceso(5);
		 espacioDisponible += proceso->cantidadDePaginas;
		 limpiarPosiciones (espacio, proceso->posPagina, proceso->cantidadDePaginas);
		 sacarElemento(5);
		 printf("Proceso eliminado exitosamente\n");
		 free(proceso);
		}
		else
		{
			printf("Proceso no encontrado\n");
		}

	if (estaElProceso(5)) printf("Test de eliminacion no fue superado\n");
	else printf("Test de eliminacion fue superado\n");
	if (estaEnArray(proceso1)) printf("Test de eliminacion no fue superado\n");
	else printf("Test de eliminacion fue superado\n");

	espacioDisponible = config.cantidad_paginas;
    limpiarPosiciones (espacio,0,config.cantidad_paginas);
    list_clean(espacioUtilizado);
}

void testFinalizarProceso2() //ESta el proceso, Se usa FinalizarProceso
{
	printf("******************testFinalizarProceso2 ha comenzado***********************\n");
	t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso1->pid = 5;
	proceso1->posPagina = 0;
	proceso1->cantidadDePaginas = 3;
	list_add(espacioUtilizado,(void*) proceso1);

	t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso2->pid = 8;
	proceso2->posPagina = 8;
	proceso2->cantidadDePaginas = 6;
	list_add(espacioUtilizado,(void*) proceso2);

	bitarray_set_bit(espacio, 0);
	bitarray_set_bit(espacio, 1);
	bitarray_set_bit(espacio, 2);
	bitarray_set_bit(espacio, 3);
	bitarray_clean_bit(espacio, 4);
	bitarray_set_bit(espacio, 5);
	bitarray_clean_bit(espacio, 6);
	bitarray_clean_bit(espacio, 7);
	bitarray_set_bit(espacio, 8);
	setearPosiciones (espacio,9,config.cantidad_paginas);

	finalizarProceso(5);

	if (estaElProceso(5)&&estaEnArray(proceso1)) printf("Test de eliminacion no fue superado\n");
	else printf("Test de eliminacion fue superado\n");

	espacioDisponible = config.cantidad_paginas;
		limpiarPosiciones (espacio,0,config.cantidad_paginas);
		list_clean(espacioUtilizado);
}

void testFinalizarProceso3() //No esta el proceso
{
	printf("******************testFinalizarProceso3 ha comenzado***********************\n");
	t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
		proceso1->pid = 5;
		proceso1->posPagina = 0;
		proceso1->cantidadDePaginas = 3;
		list_add(espacioUtilizado,(void*) proceso1);

		t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
		proceso2->pid = 8;
		proceso2->posPagina = 8;
		proceso2->cantidadDePaginas = 6;
		list_add(espacioUtilizado,(void*) proceso2);

		bitarray_set_bit(espacio, 0);
		bitarray_set_bit(espacio, 1);
		bitarray_set_bit(espacio, 2);
		bitarray_set_bit(espacio, 3);
		bitarray_clean_bit(espacio, 4);
		bitarray_set_bit(espacio, 5);
		bitarray_clean_bit(espacio, 6);
		bitarray_clean_bit(espacio, 7);
		bitarray_set_bit(espacio, 8);
		setearPosiciones (espacio,9,14);
		limpiarPosiciones(espacio,14,config.cantidad_paginas);

		if(estaElProceso(7))printf("Test de eliminacion:El proceso esta, algo anda mal");
		else printf("Test de eliminacion:El proceso no esta, no se va a poder eliminar\n");

		finalizarProceso(7);

		if (estaElProceso(7)) printf("Test de eliminacion no fue superado\n");
		else printf("Test de eliminacion fue superado\n");

		espacioDisponible = config.cantidad_paginas;
			limpiarPosiciones (espacio,0,config.cantidad_paginas);
			list_clean(espacioUtilizado);
}

void testAgregarProceso1()
{
	printf("******************testAgregarProceso1 ha comenzado***********************\n");
	t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso1->pid = 5;
	proceso1->posPagina = 0;
	proceso1->cantidadDePaginas = 3;
	list_add(espacioUtilizado,(void*) proceso1);

	t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso2->pid = 8;
	proceso2->posPagina = 8;
	proceso2->cantidadDePaginas = 6;
	list_add(espacioUtilizado,(void*) proceso2);

	bitarray_clean_bit(espacio, 0);
	bitarray_set_bit(espacio, 1);
	bitarray_clean_bit(espacio, 2);
	bitarray_clean_bit(espacio, 3);
	bitarray_clean_bit(espacio, 4);
	bitarray_set_bit(espacio, 5);
	bitarray_clean_bit(espacio, 6);
	bitarray_clean_bit(espacio, 7);
	bitarray_set_bit(espacio, 8);
	setearPosiciones (espacio,9,config.cantidad_paginas);

	espacioDisponible = espaciosDisponibles(espacio);
	asignarEspacioANuevoProceso(7,3);

	if(buscarMarcoInicial(8)==8) printf("todo ok\n");
	else printf("NOOOOOOOOOOO\n");
	if (estaElProceso(7)) printf("Test agregar fue superado\n");
	else printf("Test agregar no fue superado\n");
	if (testArrayOcupado(espacio,2,3))printf("Test agregar fue superado\n");
	else printf("Test agregar no fue superado\n");

	espacioDisponible = config.cantidad_paginas;
	limpiarPosiciones (espacio,0,config.cantidad_paginas);
	list_clean(espacioUtilizado);
}

void testAgregarProceso2() //TODO PROBLEMAS CON AGREGAR PAGINA YA QUE NO PASA EL IF
{
  printf("******************testAgregarProceso2 ha comenzado***********************\n");
  int algo=0;
  	int i=0;

    bitarray_set_bit(espacio, 0);
    bitarray_set_bit(espacio, 1);
    bitarray_set_bit(espacio, 2);
    bitarray_clean_bit(espacio, 3);
    bitarray_clean_bit(espacio, 4);
    bitarray_set_bit(espacio, 5);
    bitarray_set_bit(espacio, 6);
    bitarray_set_bit(espacio, 7);
    bitarray_clean_bit(espacio, 8);
    setearPosiciones (espacio,9,config.cantidad_paginas);

    t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
    	proceso1->pid = 5;
    	proceso1->posPagina = 0;
    	proceso1->cantidadDePaginas = 3;
    	list_add(espacioUtilizado,(void*) proceso1);

    t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
    	proceso2->pid = 8;
    	proceso2->posPagina = 5;
    	proceso2->cantidadDePaginas = 3;
    	list_add(espacioUtilizado,(void*) proceso2);

    	t_infoProceso* proceso3 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
    	  	proceso3->pid = 9;
    	  	proceso3->posPagina = 9;
    	  	proceso3->cantidadDePaginas = config.cantidad_paginas-proceso3->posPagina;
    	  	list_add(espacioUtilizado,(void*) proceso3);


    	t_infoProceso* procesoAImprimir=malloc(sizeof(t_infoProceso));
    	procesoAImprimir=buscarProcesoAPartirDeMarcoInicial(0);
    	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
    			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);


    	t_infoProceso*  procesoAImprimir2=malloc(sizeof(t_infoProceso));
    	procesoAImprimir2=buscarProcesoAPartirDeMarcoInicial(5);
    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
    	  		 procesoAImprimir2->posPagina, procesoAImprimir2->pid, procesoAImprimir2->cantidadDePaginas);

    	t_infoProceso*  procesoAImprimir3=malloc(sizeof(t_infoProceso));
    	  	procesoAImprimir3=buscarProcesoAPartirDeMarcoInicial(12);
    	  	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
    	  	  		 procesoAImprimir3->posPagina, procesoAImprimir3->pid, procesoAImprimir3->cantidadDePaginas);

   if(hayQueCompactar(3)) printf("Va a haber que compactar\n");


	espacioDisponible = espaciosDisponibles(espacio);

	asignarEspacioANuevoProceso(7,3);

	if (estaElProceso(7)) printf("Test agregar fue superado\n");
	else printf("Test agregar no fue superado\n");





	algo= primerEspacioLibre(espacio);
	  printf("La primera pagina libre es %d \n", algo);
	  for(i=0;i<config.cantidad_paginas;i++)
	  {
		  printf("En la posicion %d tengo el bit %d \n", i, bitarray_test_bit(espacio, i) );
	  }


	    	procesoAImprimir=buscarProceso(5);
	    	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	    			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);



	    	procesoAImprimir2=buscarProceso(8);
	    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	    	  		 procesoAImprimir2->posPagina, procesoAImprimir2->pid, procesoAImprimir2->cantidadDePaginas);

	    	procesoAImprimir3=buscarProceso(12);
	    	    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	    	    	  		 procesoAImprimir3->posPagina, procesoAImprimir3->pid, procesoAImprimir3->cantidadDePaginas);



	  espacioDisponible = config.cantidad_paginas;
	  limpiarPosiciones (espacio,0,config.cantidad_paginas);
	  list_clean(espacioUtilizado);
}

//************TESTS BETA MALISIMOS************* //TODO HAY QUE MEJORARLOS

//void escribirPaginaParaTests(int pid, int paginaAEscribir, int tamanio) {
//	//Abro el archivo de Swap
//	FILE *archivoSwap;
//	archivoSwap = fopen(config.nombre_swap, "r+");
//	if (archivoSwap == NULL) {
//		printf("Error al abrir el archivo para escribir\n");
//	}
//	char* texto = malloc(config.tamanio_pagina);
//	texto="hola";
//	//Al buffer que me envian para escribir lo lleno de ceros hasta completar el tamaño de página
//	int i;
//	for (i = 4; i < config.tamanio_pagina; i++) {
//		texto[i] = '\0';
//	}
//	//Me posiciono en la página que quiero escribir y escribo
//	int marcoInicial = buscarMarcoInicial(pid);
//	int marcoAEscribir = (marcoInicial + paginaAEscribir); //
//	fseek(archivoSwap, marcoAEscribir, SEEK_SET);
//	printf("texto:%s\n", texto);
//	int exitoAlEscribir = fwrite(texto, config.tamanio_pagina, 1, archivoSwap);
//
//	fclose(archivoSwap);
//	usleep(config.retardo_acceso);
//
//    //Chequeo si escribió
//	if (exitoAlEscribir == 1) {
//	    printf("Pagina escrita exitosamente\n");
//		send_w(cliente, headerToMSG(HeaderEscrituraCorrecta),1 );
//		log_info(activeLogger, "El Programa %d - Pagina Inicial:%d Tamanio:%d Contenido:%s. Escritura realizada correctamente.",
//					pid, marcoInicial, tamanio, texto);
//	} else {
//		printf("Error al escribir pagina\n");
//		send_w(cliente, headerToMSG(HeaderEscrituraErronea), 1);
//	}
//
//}
//void testLectura2()
//{
//	t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
//	proceso1->pid = 5;
//	proceso1->posPagina = 0;
//	proceso1->cantidadDePaginas = 1;
//	list_add(espacioUtilizado,(void*) proceso1);
//
//	escribirPaginaParaTests(5,0,5);
//}

void testLectura2()
{
	printf("******************testLectura2 ha comenzado***********************\n");
	t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	proceso1->pid = 5;
	proceso1->posPagina = 0;
	proceso1->cantidadDePaginas = 1;
	list_add(espacioUtilizado,(void*) proceso1);


	char* texto = malloc(config.tamanio_pagina);
	texto="pepito";
	//int i;

//	for (i = 5; i < config.tamanio_pagina; i++) { //TIRA SEGMENT FAULT
//		texto[i] = '\0';
//	}


	FILE *archivoSwap;
	archivoSwap = fopen(config.nombre_swap, "r+");
	if (archivoSwap == NULL) {
		printf("Error al abrir el archivo para escribir\n");
	}

	fseek(archivoSwap, 0, SEEK_SET);
	int exitoAlEscribir = fwrite(texto, config.tamanio_pagina, 1, archivoSwap);
	printf("El texto dice:  %s\n", texto);

	if (exitoAlEscribir) printf("Se pudo escribir\n");
    fclose(archivoSwap);
	leerPagina(5,0);

	espacioDisponible = config.cantidad_paginas;
	limpiarPosiciones (espacio,0,config.cantidad_paginas);
	list_clean(espacioUtilizado);

}

void testLectura3()
{
	printf("******************testLectura3 ha comenzado***********************\n");
	crear_archivo();
		t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
		proceso1->pid = 5;
		proceso1->posPagina = 0;
		proceso1->cantidadDePaginas = 1;
		list_add(espacioUtilizado,(void*) proceso1);

		t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	    proceso2->pid = 8;
	    proceso2->posPagina = 5;
		proceso2->cantidadDePaginas = 3;
		list_add(espacioUtilizado,(void*) proceso2);

		char* texto = malloc(config.tamanio_pagina);
		texto="holass";



		FILE *archivoSwap;
		archivoSwap = fopen(config.nombre_swap, "r+");
		if (archivoSwap == NULL) {
			printf("Error al abrir el archivo para escribir\n");
		}

		fseek(archivoSwap, 5, SEEK_SET);
		int exitoAlEscribir = fwrite(texto, config.tamanio_pagina, 1, archivoSwap);
		printf("El texto dice:  %s\n", texto);

		if (exitoAlEscribir) printf("Se pudo escribir\n");
	    fclose(archivoSwap);
		leerPagina(8,5);

		espacioDisponible = config.cantidad_paginas;
		limpiarPosiciones (espacio,0,config.cantidad_paginas);
		list_clean(espacioUtilizado);
}

void testLectura4()
{
	crear_archivo();
	int algo=0;
		int i=0;
	  printf("******************testSwapDeCompactacion4 ha comenzado***********************\n");
	  bitarray_set_bit(espacio, 0);
	  bitarray_set_bit(espacio, 1);
	  bitarray_set_bit(espacio, 2);
	  bitarray_clean_bit(espacio, 3);
	  bitarray_clean_bit(espacio, 4);
	  bitarray_set_bit(espacio, 5);
	  bitarray_set_bit(espacio, 6);
	  bitarray_set_bit(espacio, 7);
	  bitarray_clean_bit(espacio, 8);
	  setearPosiciones (espacio,9,config.cantidad_paginas);

	  t_infoProceso* proceso1 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	  	proceso1->pid = 5;
	  	proceso1->posPagina = 0;
	  	proceso1->cantidadDePaginas = 3;
	  	list_add(espacioUtilizado,(void*) proceso1);

	  t_infoProceso* proceso2 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	  	proceso2->pid = 8;
	  	proceso2->posPagina = 5;
	  	proceso2->cantidadDePaginas = 3;
	  	list_add(espacioUtilizado,(void*) proceso2);

	  	t_infoProceso* proceso3 = (t_infoProceso*) malloc(sizeof(t_infoProceso));
	  	  	proceso3->pid = 9;
	  	  	proceso3->posPagina = 9;
	  	  	proceso3->cantidadDePaginas = config.cantidad_paginas-proceso3->posPagina;
	  	  	list_add(espacioUtilizado,(void*) proceso3);


	  	t_infoProceso* procesoAImprimir=malloc(sizeof(t_infoProceso));
	  	procesoAImprimir=buscarProcesoAPartirDeMarcoInicial(0);
	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	  			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);


	  	t_infoProceso*  procesoAImprimir2=malloc(sizeof(t_infoProceso));
	  	procesoAImprimir2=buscarProcesoAPartirDeMarcoInicial(5);
	  	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	  	  		 procesoAImprimir2->posPagina, procesoAImprimir2->pid, procesoAImprimir2->cantidadDePaginas);

	  	t_infoProceso*  procesoAImprimir3=malloc(sizeof(t_infoProceso));
	  	  	procesoAImprimir3=buscarProcesoAPartirDeMarcoInicial(12);
	  	  	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	  	  	  		 procesoAImprimir3->posPagina, procesoAImprimir3->pid, procesoAImprimir3->cantidadDePaginas);




	  	  	char* texto = malloc(config.tamanio_pagina);
	  	  		texto="hola";
	  	  	FILE *archivoSwap;
	  	  		archivoSwap = fopen(config.nombre_swap, "r+");
	  	  		if (archivoSwap == NULL) {
	  	  			printf("Error al abrir el archivo para escribir\n");
	  	  		}

	  	  		fseek(archivoSwap, 5, SEEK_SET);
	  	  		int exitoAlEscribir = fwrite(texto, config.tamanio_pagina, 1, archivoSwap);
	  	  		printf("El texto dice:  %s\n", texto);

	  	  		if (exitoAlEscribir) printf("Se pudo escribir\n");
	  	  	    fclose(archivoSwap);

	  	  		leerPagina(8,5);




	  compactar();

	//  if(hayQueCompactar(3)) printf("Test de posibilidad de compactacion no fue superado\n");
	//  else printf("Test de posibilidad de compactacion fue superado\n");
	  algo= primerEspacioLibre(espacio);
	  printf("La primera pagina libre es %d \n", algo);
	  for(i=0;i<config.cantidad_paginas;i++)
	  {
		  printf("En la posicion %d tengo el bit %d \n", i, bitarray_test_bit(espacio, i) );
	  }


	    	procesoAImprimir=buscarProceso(5);
	    	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	    			procesoAImprimir->posPagina,procesoAImprimir->pid,procesoAImprimir->cantidadDePaginas);



	    	procesoAImprimir2=buscarProceso(8);
	    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	    	  		 procesoAImprimir2->posPagina, procesoAImprimir2->pid, procesoAImprimir2->cantidadDePaginas);

	    	procesoAImprimir3=buscarProceso(12);
	    	    	  	printf("el proceso en el marco %d es el %d que ocupa %d paginas\n",
	    	    	  		 procesoAImprimir3->posPagina, procesoAImprimir3->pid, procesoAImprimir3->cantidadDePaginas);




	    	 leerPagina(8,0);


	  espacioDisponible = config.cantidad_paginas;
	  limpiarPosiciones (espacio,0,config.cantidad_paginas);
	  list_clean(espacioUtilizado);

}
