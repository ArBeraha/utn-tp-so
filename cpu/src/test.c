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

void test_cantidad_paginas_ocupa() {
	log_debug(bgLogger, "INICIO cantidad_paginas_ocupa");
	int aux = tamanioPaginas;
	t_sentencia sentencia;

	tamanioPaginas=4;
	sentencia.offset_inicio=20;
	sentencia.offset_fin=41; //ocupa 21, osea 5,25 pags, osea 6 pags.
	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),6);

	tamanioPaginas=20;	//ahora ocuparia 2 paginas. Una completa (20/20) y casi nada de otra (1/20)
	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),2);

	tamanioPaginas=8000;
	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),1);

	tamanioPaginas=4;
	sentencia.offset_fin=40; //ocupa 20, osea 5 pags.
	CU_ASSERT_EQUAL(cantidad_paginas_ocupa(&sentencia),5);

	tamanioPaginas = aux;
	log_debug(bgLogger, "FIN cantidad_paginas_ocupa");
}

int test_cpu() {
	log_info(activeLogger, "INICIANDO TESTS DE CPU");
	CU_initialize_registry();
	CU_pSuite suite_nucleo = CU_add_suite("Suite de CPU", NULL, NULL);
	CU_add_test(suite_nucleo, "Test obtener_offset_relativo.",
				test_obtener_offset_relativo);
	CU_add_test(suite_nucleo, "Test cantidad_paginas_ocupa.",
			test_cantidad_paginas_ocupa);
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
