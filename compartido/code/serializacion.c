/*
 * serializacion.c
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#include "serializacion.h"

void imprimir_serializacion(char* serial, int largo) {
	int i;
	for (i = 0; i < largo; i++) {
		printf("[%d]", serial[i]);
	}
	printf("\n");
}
int serializar_int(char* destino, int* fuente) {
	memcpy(destino, fuente, sizeof(int));
	return sizeof(int);
}
int deserializar_int(int* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(int));
	return sizeof(int);
}
int serializar_variable(char* destino, t_variable* fuente) {
	memcpy(destino, fuente, sizeof(t_variable));
	return sizeof(t_variable);
}
int deserializar_variable(t_variable* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(t_variable));
	return sizeof(t_variable);
}
int serializar_sentencia(char* destino, t_sentencia* fuente) {
	memcpy(destino, fuente, sizeof(t_sentencia));
	return sizeof(t_sentencia);
}
int deserializar_sentencia(t_sentencia* destino, char* fuente) {
	memcpy(destino, fuente, sizeof(t_sentencia));
	return sizeof(t_sentencia);

}
int serializar_list(char* destino, t_list* fuente, int pesoElemento) {
	int i, offset = 1;
	destino[0] = list_size(fuente); // Se va a guardar en la primera posicion la cantidad de elementos
	for (i = 0; i < destino[0]; i++) {
		memcpy(destino + offset, list_get(fuente, i), pesoElemento);
		offset += pesoElemento;
	}
	return offset;
}
int deserializar_list(t_list* destino, char* fuente, int pesoElemento) {
	int i, offset = 1; // Se va a guardar en la primera posicion la cantidad de elementos
	char* buffer;
	for (i = 0; i < fuente[0]; i++) {
		buffer = malloc(pesoElemento);
		memcpy(buffer, fuente + offset, pesoElemento);
		list_add(destino, buffer);
		offset += pesoElemento;
	}
	return offset;
}
int bytes_list(t_list* fuente, int pesoElemento){
	return list_size(fuente)*pesoElemento+1;
}
int bytes_stack_item(t_stack_item* fuente) {
	return sizeof(int) + bytes_list(fuente->argumentos, sizeof(t_variable))
			+ bytes_list(fuente->identificadores, sizeof(t_identificador))
			+ sizeof(int) + sizeof(t_variable);
}
int bytes_stack(t_stack* fuente) {
	int i, bytes = 0;
	for (i = 0; i < stack_size(fuente); i++) {
		bytes += bytes_stack_item(list_get(fuente, i));
	}
	return bytes;
}
int serializar_stack_item(char* destino, t_stack_item* fuente) {
	int offset = 0;
	offset += serializar_int(destino, &(fuente->posicion));
	offset += serializar_list(destino + offset, fuente->argumentos,	sizeof(t_variable));
	offset += serializar_list(destino + offset, fuente->identificadores, sizeof(t_identificador));
	offset += serializar_int(destino, &(fuente->posicionRetorno));
	offset += serializar_variable(destino, &(fuente->valorRetorno));
	return offset; // Retorna el offset
}
int serializar_stack(char* destino, t_stack* fuente) {
	int i, offset = 1;
	destino[0] = stack_size(fuente); // Cantidad de items
	for (i = 0; i < destino[0]; i++) {
		offset += serializar_stack_item(destino + offset, list_get(fuente, i)); // Serial del item
	}
	return offset;
}
int deserializar_stack_item(t_stack_item* destino, char* fuente) {
	int offset = 0;
	destino->argumentos=list_create();
	destino->identificadores=list_create();
	offset += deserializar_int(&destino->posicion, fuente + offset);
	offset += deserializar_list(destino->argumentos, fuente + offset, sizeof(t_variable));
	offset += deserializar_list(destino->identificadores, fuente + offset, sizeof(t_identificador));
	offset += deserializar_int(&destino->posicionRetorno, fuente + offset);
	offset += deserializar_variable(&(destino->valorRetorno), fuente + offset);
	return offset;
}
int deserializar_stack(t_stack* destino, char* fuente) {
	t_stack_item* item;
	int i, offset = 1; // Cantidad de items en fuente[0]
	for (i = 0; i < fuente[0]; i++) {
		item = malloc(sizeof(t_stack_item)); // Tamaño del item (o sea de sus punteros tambien [NO DE LAS LISTAS EN SI])
		offset += deserializar_stack_item(item, fuente + offset);
		stack_push(destino, item); // Agregar el item recibido al stack
	}
	return offset;
}

/* INICIO DE TESTS */
int test_serializacion(){
	CU_initialize_registry();
	CU_pSuite suite_serializacion = CU_add_suite("Suite de Serializacion", NULL, NULL);
	CU_add_test(suite_serializacion, "Test de Serializacion de int", test_serializar_int);
	CU_add_test(suite_serializacion, "Test de Serializacion de variable", test_serializar_variable);
	CU_add_test(suite_serializacion, "Test de Serializacion de sentencia", test_serializar_sentencia);
	CU_add_test(suite_serializacion, "Test de Serializacion de list", test_serializar_list);
	CU_add_test(suite_serializacion, "Test de Serializacion de stack item", test_serializar_stack_item);
	CU_add_test(suite_serializacion, "Test de Serializacion de stack", test_serializar_stack);
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
void test_serializar_int(){
	int a=57,b;
	char* serial;
	serial = malloc(sizeof(int));
	serializar_int(serial,&a);
	imprimir_serializacion(serial,sizeof(int));
	deserializar_int(&b,serial);
	CU_ASSERT_EQUAL(a,b);
	CU_ASSERT_EQUAL(b,57);
	free(serial);
}
void test_serializar_variable(){
	t_variable* varA=malloc(sizeof(t_variable));
	t_variable*	varB=malloc(sizeof(t_variable));
	char* serial=malloc(sizeof(t_variable));
	serializar_variable(serial,varA);
	imprimir_serializacion(serial,sizeof(t_variable));
	deserializar_variable(varB,serial);
	CU_ASSERT_EQUAL(varA->offset,varB->offset);
	CU_ASSERT_EQUAL(varA->pagina,varB->pagina);
	CU_ASSERT_EQUAL(varA->size,varB->size);
	free(varA);
	free(varB);
	free(serial);
}
void test_serializar_sentencia(){
	t_sentencia* senA=malloc(sizeof(t_sentencia));
	t_sentencia* senB=malloc(sizeof(t_sentencia));
	senA->offset_inicio=1;
	senA->offset_fin=2;
	char* serial=malloc(sizeof(t_sentencia));
	serializar_sentencia(serial,senA);
	imprimir_serializacion(serial,sizeof(t_sentencia));
	deserializar_sentencia(senB,serial);
	CU_ASSERT_EQUAL(senA->offset_inicio,senB->offset_inicio);
	CU_ASSERT_EQUAL(senA->offset_fin,senB->offset_fin);
	free(senA);
	free(senB);
	free(serial);
}
void test_serializar_list(){
	t_list *listA, *listB = NULL;
	listA = list_create();
	listB = list_create();
	t_variable* var = malloc(sizeof(t_variable));
	var->pagina=2;
	list_add(listA,var);
	char *serial = malloc(bytes_list(listA,sizeof(t_variable)));
	serializar_list(serial,listA,sizeof(t_variable));
	imprimir_serializacion(serial,sizeof(t_variable));
	deserializar_list(listB,serial,sizeof(t_variable));
	CU_ASSERT_EQUAL(((t_variable*)list_get(listB,0))->pagina,2);
	list_destroy(listA);
	list_destroy(listB);
	free(var);
	free(serial);
}
void test_serializar_stack_item(){
	/*
	t_stack_item* itemA, *itemB;
	t_variable var;
	var.offset=1;
	var.pagina=2;
	var.size=3;
	itemA = malloc(sizeof(t_stack_item));
	itemB = malloc(sizeof(t_stack_item));
	itemA->posicion=5;
	itemA->argumentos = list_create();
	itemA->identificadores = list_create();
	itemA->valorRetorno=var;
	char* serial = malloc(bytes_stack_item(itemA));
	serializar_stack_item(serial,itemA);
	imprimir_serializacion(serial,bytes_stack_item(itemA));
	return;
	deserializar_stack_item(itemB,serial);
	CU_ASSERT_EQUAL(itemB->posicion,5)
	*/
}
void test_serializar_stack(){
	/*
	t_stack* stack = stack_create();
	t_stack* stackB = stack_create();
	t_stack_item* item = malloc(sizeof(t_stack_item));
	item->posicion=5;
	stack_push(stack,item);
	char* serial = malloc(bytes_stack(stack));
	serializar_stack(serial,stack);
	deserializar_stack(stackB,serial);
	CU_ASSERT_EQUAL(((t_stack_item *)stack_pop(stackB))->posicion,5);*/
}




