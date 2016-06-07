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
	fuente.offset_inicio=21; // 5*4+1
	fuente.offset_fin=40; // 10*4+0
	int nroPag = obtener_offset_relativo(&fuente, &destino);
	CU_ASSERT_EQUAL(nroPag,5);
	CU_ASSERT_EQUAL(destino.offset_inicio,1);
	CU_ASSERT_EQUAL(destino.offset_fin,20);
	tamanioPaginas = aux;
	log_debug(bgLogger, "FIN test_obtener_offset_relativo");
}

int test_cpu() {
	log_info(activeLogger, "INICIANDO TESTS DE CPU");
	CU_initialize_registry();
	CU_pSuite suite_nucleo = CU_add_suite("Suite de CPU", NULL, NULL);
	CU_add_test(suite_nucleo, "Test obtener_offset_relativo: dirAbsoluta -> dirRelativa",
			test_obtener_offset_relativo);
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
