/*
 * test.c
 *
 *  Created on: 4/6/2016
 *      Author: utnso
 */
#include "swap.h"
#include <CUnit/Basic.h>

int test_swap() {
	log_info(activeLogger, "INICIANDO TESTS DE SWAP");
	CU_initialize_registry();
	CU_pSuite suite_swap = CU_add_suite("Suite de Swap", NULL, NULL);
	CU_add_test(suite_swap,
			"Test de Escritura y Lectura Estatica (Sin compactacion)",
			test_escrituraYLecturaEstatica);
	CU_add_test(suite_swap, "Test de llenado del bitarray",
			test_asignacionProcesos);
	CU_add_test(suite_swap,
			"Test de Escritura y Lectura Dinamica (Con compactacion)",
			test_escrituraYLecturaDinamica);
	CU_add_test(suite_swap, "Test de Compactacion", test_compactacion);
	CU_add_test(suite_swap, "Test de coherencia del espacio disponible",
			test_espacioDisponibleYFinalizarProceso);
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	log_info(activeLogger, "FINALIZADO TESTS DE SWAP");
	return CU_get_error();
}
int testear(int(*suite)(void)){
	if (suite() != CUE_SUCCESS) {
		printf("%s", CU_get_error_msg());
		return EXIT_FAILURE;
	}
	return 0;
}
///****************************************TESTS***************************************************************/

void test_escrituraYLecturaEstatica(){
	char* s2, *s1 = malloc(config.tamanio_pagina);
	int i,a=48;
	for (i=0;i<config.tamanio_pagina;i++){
		s1[i]=a++;
		if (a==58)
			a=48;
	}
	escribirPagina(0,s1);
    s2 = leerPagina(0);
    CU_ASSERT_TRUE(strcmp(s1,s2)==0);
    free(s1);
    free(s2);
    limpiarEstructuras();
}
void test_asignacionProcesos(){
	asignarEspacioANuevoProceso(1,5);
	asignarEspacioANuevoProceso(2,5);
	CU_ASSERT_EQUAL(espacioDisponible,config.cantidad_paginas-10);
	CU_ASSERT_EQUAL(list_size(espacioUtilizado),2);
	limpiarEstructuras();
}
void test_compactacion(){
	asignarEspacioANuevoProceso(1,7);
	asignarEspacioANuevoProceso(2,10);
	asignarEspacioANuevoProceso(3,9);
	finalizarProceso(1);
	compactar();
	CU_ASSERT_NOT_EQUAL(primerEspacioLibre(),0);
	CU_ASSERT_EQUAL(buscarProcesoSegunInicio(10)->pid,3);
	limpiarEstructuras();
}
void test_escrituraYLecturaDinamica(){
	asignarEspacioANuevoProceso(1,7);
	asignarEspacioANuevoProceso(2,10);
	asignarEspacioANuevoProceso(3,3);
	asignarEspacioANuevoProceso(4,5);
	escribirPagina(7,"Hola");
	escribirPagina(20,"Chau");
	finalizarProceso(1);
	finalizarProceso(3);
	compactar();
	char* s1 = leerPagina(0); 	//La primer pagina del pid 2 pasa al marco 0
	char* s2 = leerPagina(10); 	//La primer pagina del pid 4 pasa al marco 10
	CU_ASSERT_TRUE(strcmp(s1,"Hola"));
	CU_ASSERT_TRUE(strcmp(s2,"Chau"));
	free(s1);
	free(s2);
	limpiarEstructuras();
}
void test_espacioDisponibleYFinalizarProceso(){
	int espacioInicial = config.cantidad_paginas;
	asignarEspacioANuevoProceso(1,7);
	CU_ASSERT_EQUAL(espacioDisponible,espacioInicial-(7));
	asignarEspacioANuevoProceso(2,10);
	asignarEspacioANuevoProceso(3,3);
	asignarEspacioANuevoProceso(4,5);
	CU_ASSERT_EQUAL(espacioDisponible,espacioInicial-(7+10+3+5));
	finalizarProceso(1);
	CU_ASSERT_EQUAL(espacioDisponible,espacioInicial-(10+3+5));
	compactar();
	CU_ASSERT_EQUAL(espacioDisponible,espacioInicial-(10+3+5));
	moverPagina(0,20);
	CU_ASSERT_EQUAL(espacioDisponible,espacioInicial-(10+3+5));
	finalizarProceso(2);
	finalizarProceso(3);
	finalizarProceso(4);
	CU_ASSERT_FALSE(estaProceso(1));
	CU_ASSERT_FALSE(estaProceso(2));
	CU_ASSERT_FALSE(estaProceso(3));
	CU_ASSERT_FALSE(estaProceso(4));
	CU_ASSERT_EQUAL(espacioDisponible,espacioInicial);
}
