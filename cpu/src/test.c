/*
 * test.c
 *
 *  Created on: 6/6/2016
 *      Author: utnso
 */
#include <CUnit/Basic.h>
#include "cpu.h"

void test_obtener_offset_relativo() {
	log_debug(bgLogger, "INICIO test_obtener_offset_relativo");
	int aux = tamanioPaginas;
	t_sentencia fuente, destino;
	tamanioPaginas=4;
	fuente.offset_inicio=21; // 5*tamanioPaginas+1
	fuente.offset_fin=40; // 10*tamanioPaginas+0
	int nroPag = obtener_offset_relativo(&fuente, &destino);
	CU_ASSERT_EQUAL(nroPag,5);
	CU_ASSERT_EQUAL(destino.offset_inicio,1);
	CU_ASSERT_EQUAL(destino.offset_fin,20);
	tamanioPaginas = aux;
	log_debug(bgLogger, "FIN test_obtener_offset_relativo");
}

/**
 * Esta funcion ya no es necesaria.
 */
//void test_cantidad_paginas_ocupa() {
//	log_debug(bgLogger, "INICIO cantidad_paginas_ocupa");
//	int aux = tamanioPaginas;
//	t_sentencia sentencia;
//
//	tamanioPaginas=4;
//	sentencia.offset_inicio=20;
//	sentencia.offset_fin=41; //ocupa 21, osea 5,25 pags, osea 6 pags.
//	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),6);
//
//	tamanioPaginas=20;	//ahora ocuparia 2 paginas. Una completa (20/20) y casi nada de otra (1/20)
//	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),2);
//
//	tamanioPaginas=8000;
//	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),1);
//
//	tamanioPaginas=4;
//	sentencia.offset_fin=40; //ocupa 20, osea 6 pags.
//	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),6);
//
//	tamanioPaginas = aux;
//	log_debug(bgLogger, "FIN cantidad_paginas_ocupa");
//}

void test_envio_solicitudes_una_pagina(){
	log_info(bgLogger,"Test de envio de solicitudes en una pagina");

	tamanioPaginas=10;
	t_sentencia* sentencia = malloc(sizeof(t_sentencia));

	sentencia->offset_inicio = 2;			//el pedido es offset:2, fin:5, size:3, pag:0
	sentencia->offset_fin= 5;

	t_sentencia* sentenciaAux = malloc(sizeof(t_sentencia));

	int pagina = obtener_offset_relativo(sentencia,sentenciaAux);
	int longitud_aux = longitud_sentencia(sentenciaAux);

	CU_ASSERT_EQUAL(pagina,0);
	CU_ASSERT_EQUAL(longitud_aux,3);

	free(sentencia);
	free(sentenciaAux);
	log_debug(bgLogger, "FIN solicitudes_una_pagina");
}

void test_envio_solicitudes_varias_paginas(){

	log_info(bgLogger,"Test de envio de solicitudes en varias paginas");

	tamanioPaginas=4;
	t_sentencia* sentencia = malloc(sizeof(t_sentencia));
	t_sentencia* sentenciaAux = malloc(sizeof(t_sentencia));

	sentencia->offset_inicio = 4;
	sentencia->offset_fin= 15;
	// [t_sentencia] (4,15) -> (1,0,11) [t_pedido]
	// 1*tamPag + 0 = 4
	// 1*tamPag + 11 = 15

	int pagina = obtener_offset_relativo(sentencia,sentenciaAux);
	CU_ASSERT_EQUAL(pagina,1);
	CU_ASSERT_EQUAL(sentenciaAux->offset_inicio,0);
	CU_ASSERT_EQUAL(sentenciaAux->offset_fin,11);

	log_debug(bgLogger, "FIN test_envio_solicitudes_varias_paginas");
}

void test_definir_variable(){
	log_debug(bgLogger, "Test primitiva definir variable");
	tamanioPaginas = 4;

	pcbActual = malloc(sizeof(t_PCB));
	pcbActual->PC = 0;
	stack =stack_create();

	t_stack_item* item = malloc(sizeof(t_stack_item));
	item->identificadores =  dictionary_create();
	stack_push(stack,item);

	definir_variable('a');
	item = stack_pop(stack);

	CU_ASSERT_EQUAL(pcbActual->PC,1);
	CU_ASSERT(dictionary_has_key(item->identificadores, "a"));

	log_debug(bgLogger, "FIN test_definir_variable");
}



int test_cpu() {
	log_info(activeLogger, "INICIANDO TESTS DE CPU");
	CU_initialize_registry();
	CU_pSuite suite_nucleo = CU_add_suite("Suite de CPU", NULL, NULL);
	CU_add_test(suite_nucleo, "Test obtener_offset_relativo.",	test_obtener_offset_relativo);
	//CU_add_test(suite_nucleo, "Test cantidad_paginas_ocupa.", test_cantidad_paginas_ocupa);
	CU_add_test(suite_nucleo, "Test envio_solicitudes_una_pagina",test_envio_solicitudes_una_pagina);
	CU_add_test(suite_nucleo, "Test envio_solicitudes_varias_paginas",test_envio_solicitudes_varias_paginas);
	CU_add_test(suite_nucleo, "Test definir_variables",test_definir_variable);

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	log_info(activeLogger, "FINALIZADO TESTS DE CPU");
	return CU_get_error();
}

int testear(int(*suite)(void)){
	if (suite() != CUE_SUCCESS) {
		printf("%s", CU_get_error_msg());
		return EXIT_FAILURE;
	}
	return 0;
}
