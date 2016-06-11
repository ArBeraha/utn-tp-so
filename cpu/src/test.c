/*
 * test.c
 *
 *  Created on: 6/6/2016
 *      Author: utnso
 */
#include <CUnit/Basic.h>
#include "cpu.h"
#include "primitivas.h"

/*------FUNCIONES DE INICIALIZACION / FINALIZACION -------------*/
void init(){
	pcbActual = malloc(sizeof(t_PCB));

	stack =stack_create();
	t_stack_item* item = malloc(sizeof(t_stack_item));
	item->identificadores =  dictionary_create();
	stack_push(stack,item);

	pcbActual->indice_etiquetas = dictionary_create();
}

void fin(){

	dictionary_clean(pcbActual->indice_etiquetas);
	dictionary_destroy(pcbActual->indice_etiquetas);

	stack_destroy(stack);
	free(pcbActual);
}

void test_obtener_offset_relativo() {
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
}

void test_envio_solicitudes_una_pagina(){

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
}

void test_envio_solicitudes_varias_paginas(){

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

}

void test_definir_variable(){

	pcbActual->PC = 0;
	tamanioPaginas = 4;
	definir_variable('a');

	t_stack_item* item = stack_pop(stack);

	CU_ASSERT_EQUAL(pcbActual->PC,1);
	CU_ASSERT(dictionary_has_key(item->identificadores, "a"));

	stack_push(stack,item);

	definir_variable('b');
	t_stack_item* otroItem = stack_pop(stack);
	CU_ASSERT_EQUAL(pcbActual->PC,2);
	CU_ASSERT(dictionary_has_key(otroItem->identificadores,"b"));

}

void test_ir_al_label(){ //TODO rompe la primitiva
	pcbActual->PC = 0;
	irAlLabel("goku"); //JAJAJA el nombre
	CU_ASSERT_EQUAL(pcbActual->PC,-1);

	//	dictionary_put(pcbActual->indice_etiquetas,"double", (void *)3);
	//
	//irAlLabel("double");
	//CU_ASSERT_EQUAL(pcbActual->PC,3);

}


void asignar_y_dereferenciar(){
	tamanioPaginas = 64;
	//No pedir de la primera pagina x ahora xq ta en swap para el test.

	int direccion = 64;
	t_valor_variable valorReal=25;
	asignar(direccion, valorReal); //Asigno el valor 25 en la direccion 64: (nPag, offset, size) = (1, 0, 4)
	int valorRecibido = dereferenciar(direccion);
	CU_ASSERT_EQUAL(valorReal,valorRecibido);

	direccion = 128;
	valorReal=0;
	asignar(direccion, valorReal);  //Asigno el valor 0 en la direccion 128: (nPag, offset, size) = (2, 0, 4)
	valorRecibido = dereferenciar(direccion);
	CU_ASSERT_EQUAL(valorReal,valorRecibido);

	direccion = 140;
	valorReal=-6;
	asignar(direccion, valorReal); //Asigno el valor -6 en la direccion 140: (nPag, offset, size) = (2, 12, 4)
	valorRecibido = dereferenciar(direccion);
	CU_ASSERT_EQUAL(valorReal,valorRecibido);
}


int test_cpu() {
	log_info(activeLogger, "INICIANDO TESTS DE CPU");
	CU_initialize_registry();

	CU_pSuite suite_cpu = CU_add_suite("Suite de CPU", NULL, NULL);
	CU_add_test(suite_cpu, "Test obtener_offset_relativo.",	test_obtener_offset_relativo);
	CU_add_test(suite_cpu, "Test envio_solicitudes_una_pagina",test_envio_solicitudes_una_pagina);
	CU_add_test(suite_cpu, "Test envio_solicitudes_varias_paginas",test_envio_solicitudes_varias_paginas);
	CU_add_test(suite_cpu, "Test primitiva #1: definir_variables",test_definir_variable);
	CU_add_test(suite_cpu, "Test primitiva #7: ir_al_label",test_ir_al_label);

	init();

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

	fin();

	CU_cleanup_registry();
	log_info(activeLogger, "FINALIZADO TESTS DE CPU");
	return CU_get_error();
}

int test_cpu_con_umc() {
	log_info(activeLogger, "INICIANDO TESTS DE CPU CON UMC");
	CU_initialize_registry();

	CU_pSuite suite_cpu = CU_add_suite("Suite de CPU con UMC", NULL, NULL);
	CU_add_test(suite_cpu, "Test primitivas #3 y #4: Dereferenciar y asignar.",	asignar_y_dereferenciar);

	init();

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

	fin();

	CU_cleanup_registry();
	log_info(activeLogger, "FINALIZADO TESTS DE CPU CON UMC");
	return CU_get_error();
}

int testear(int(*suite)(void)){
	if (suite() != CUE_SUCCESS) {
		printf("%s", CU_get_error_msg());
		return EXIT_FAILURE;
	}
	return 0;
}
