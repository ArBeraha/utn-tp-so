/*
 * serializacion.c
 *
 *  Created on: 11/5/2016
 *      Author: utnso
 */

#include "serializacion.h"

void imprimir_serializacion(char* serial, int largo) {
	int i;
	printf("\nserial=");
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
int serializar_pint(char* destino, int fuente) {
	memcpy(destino, &fuente, sizeof(int));
	return sizeof(int);
}
int deserializar_pint(int destino, char* fuente) {
	memcpy(&destino, fuente, sizeof(int));
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
	return 1+list_size(fuente)*pesoElemento;
}
int bytes_stack_item(t_stack_item* fuente) {
	return sizeof(int) + bytes_list(fuente->argumentos, sizeof(t_variable))
			+ bytes_dictionary(fuente->identificadores, sizeof(t_variable))//bytes_list(fuente->identificadores, sizeof(t_identificador))
			+ sizeof(int) + sizeof(t_variable);
}
int bytes_stack(t_stack* fuente) {
	int i, bytes = 1;
	for (i = 0; i < stack_size(fuente); i++) {
		bytes += bytes_stack_item(list_get(fuente, i));
	}
	return bytes;
}
int serializar_stack_item(char* destino, t_stack_item* fuente) {
	int offset = 0;
	offset += serializar_int(destino + offset, &(fuente->posicion));
	offset += serializar_list(destino + offset, fuente->argumentos,	sizeof(t_variable));
	//offset += serializar_list(destino + offset, fuente->identificadores, sizeof(t_identificador));
	offset += serializar_dictionary(destino + offset, fuente->identificadores, sizeof(t_variable));
	offset += serializar_int(destino + offset, &(fuente->posicionRetorno));
	offset += serializar_variable(destino + offset, &(fuente->valorRetorno));
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
	destino->identificadores=dictionary_create();
	offset += deserializar_int(&destino->posicion, fuente + offset);
	offset += deserializar_list(destino->argumentos, fuente + offset, sizeof(t_variable));
	offset += deserializar_dictionary(destino->identificadores, fuente + offset, sizeof(t_variable));
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
int bytes_PCB(t_PCB* pcb) {
	return sizeof(int) * 3 + bytes_stack(pcb->SP)
			+ bytes_list(pcb->indice_codigo, sizeof(t_sentencia))
			+ bytes_dictionary(pcb->indice_etiquetas, sizeof(int));
}
int serializar_PCB(char* destino,t_PCB* fuente){
	int offset=0;
	offset+=serializar_int(destino+offset,&(fuente->PC));
	offset+=serializar_int(destino+offset,&(fuente->PID));
	offset+=serializar_stack(destino+offset,fuente->SP);
	offset+=serializar_int(destino+offset,&(fuente->cantidad_paginas));
	offset+=serializar_list(destino+offset,fuente->indice_codigo,sizeof(t_sentencia));
	offset+=serializar_dictionary(destino+offset,fuente->indice_etiquetas,sizeof(int));
	return offset;
}
int deserializar_PCB(t_PCB* destino, char* fuente){
	int offset=0;
	destino->indice_codigo = list_create();
	destino->indice_etiquetas = dictionary_create();
	destino->SP=stack_create();
	offset+=deserializar_int(&(destino->PC),fuente+offset);
	offset+=deserializar_int(&(destino->PID),fuente+offset);
	offset+=deserializar_stack(destino->SP,fuente+offset);
	offset+=deserializar_int(&(destino->cantidad_paginas),fuente+offset);
	offset+=deserializar_list(destino->indice_codigo,fuente+offset,sizeof(t_sentencia));
	offset+=deserializar_dictionary(destino->indice_etiquetas,fuente+offset,sizeof(int));
	return 0;
};
int bytes_dictionary(t_dictionary* dic, int pesoData){
	int table_index,bytes=1; // la cantidad de elementos
	for (table_index = 0; table_index < dic->table_max_size; table_index++) {
		t_hash_element *element = dic->elements[table_index];

		while (element != NULL) {
			bytes++; // La cantidad de caracteres de la key del elemento
			bytes += strlen(element->key);
			bytes += pesoData;
			//Siguiente elemento
			element = element->next;
		}
	}
	return bytes;
}
int serializar_dictionary(char* destino, t_dictionary* self, int pesoData){
	int table_index,posta_size=0 ,offset=1;
	destino[0]=self->table_max_size;
	for (table_index = 0; table_index < destino[0]; table_index++) {
		t_hash_element *element = self->elements[table_index];

		while (element != NULL) {
			posta_size++;
			destino[offset++] = strlen(element->key);
			memcpy(destino + offset, element->key, strlen(element->key));
			offset += strlen(element->key);
			memcpy(destino + offset, element->data, pesoData);
			offset += pesoData;
			//Siguiente elemento
			element = element->next;
		}
	}
	destino[0]=posta_size; // El verdadero size, Posta
	return offset;
}
int deserializar_dictionary(t_dictionary* destino, char* fuente, int pesoData){
	int i,offset=1;
	char* key;
	void* data;
	//destino = dictionary_create();
	for (i=0; i< fuente[0]; i++){
		int len=fuente[offset++];
		key = malloc(len);
		memcpy(key,fuente+offset,len);
		offset+=len;
		data = malloc(pesoData);
		memcpy(data,fuente+offset,pesoData);
		offset+=pesoData;
		dictionary_put(destino,key,data);
		//no-free de key y data
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
	CU_add_test(suite_serializacion, "Test de Serializacion de dictionary", test_serializar_dictionary);
	CU_add_test(suite_serializacion, "Test de Serializacion de stack item", test_serializar_stack_item);
	CU_add_test(suite_serializacion, "Test de Serializacion de stack", test_serializar_stack);
	CU_add_test(suite_serializacion, "Test de Serializacion de PCB", test_serializar_PCB);

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
void test_serializar_dictionary(){
	t_dictionary* dic = dictionary_create();
	t_dictionary* dic2 = dictionary_create();
	t_variable* var = malloc(sizeof(t_variable));
	var->offset=1;
	var->pagina=2;
	var->size=3;
	dictionary_put(dic,"aaa",var);
	t_variable* var2 = malloc(sizeof(t_variable));
	var2->offset=11;
	var2->pagina=12;
	var2->size=13;
	dictionary_put(dic,"b",var2);
	int bytes = bytes_dictionary(dic,sizeof(t_variable));
	char* serial = malloc(bytes);
	serializar_dictionary(serial,dic,sizeof(t_variable));
	imprimir_serializacion(serial,bytes);
	deserializar_dictionary(dic2,serial,sizeof(t_variable));
	CU_ASSERT_EQUAL(((t_variable*)dictionary_get(dic2,"aaa"))->pagina,2);
	CU_ASSERT_EQUAL(((t_variable*)dictionary_get(dic2,"b"))->pagina,12);
}
void test_serializar_stack_item(){
	t_stack_item* itemA, *itemB;
	t_variable var;
	var.pagina=1;
	var.offset=2;
	var.size=3;
	itemA = malloc(sizeof(t_stack_item));
	itemB = malloc(sizeof(t_stack_item));
	itemA->posicion=5;
	itemA->posicionRetorno=10;
	itemA->argumentos = list_create();
	itemA->identificadores = dictionary_create();//list_create();
	itemA->valorRetorno=var;
	//itemB->argumentos=list_create();
	//itemB->identificadores = dictionary_create();
	t_variable* ident= malloc(sizeof(t_variable));
	ident->offset=7;
	ident->pagina=8;
	ident->size=9;
	dictionary_put(itemA->identificadores,"a",ident);
	char* serial = malloc(bytes_stack_item(itemA));
	serializar_stack_item(serial,itemA);
	imprimir_serializacion(serial,bytes_stack_item(itemA));
	deserializar_stack_item(itemB,serial);
	CU_ASSERT_EQUAL(itemB->posicion,itemA->posicion);
	CU_ASSERT_EQUAL(itemB->posicionRetorno,itemA->posicionRetorno);
	CU_ASSERT_EQUAL(itemB->valorRetorno.pagina,itemA->valorRetorno.pagina);
	CU_ASSERT_EQUAL(((t_variable*)dictionary_get(itemB->identificadores,"a"))->pagina,8);
	/*stack_destroy_item(itemA);
	stack_destroy_item(itemB);*/
	list_destroy(itemA->argumentos);
	dictionary_destroy(itemA->identificadores);
	free(itemA);
	list_destroy(itemB->argumentos);
	dictionary_destroy(itemB->identificadores);
	free(itemB);
	free(serial);
}
void test_serializar_stack(){
	t_stack* stackA = stack_create();
	t_stack* stackB = stack_create();
	t_variable var;
	var.pagina=1;
	var.offset=2;
	var.size=3;
	t_stack_item* itemA = malloc(sizeof(t_stack_item));
	itemA->posicion=1;
	itemA->posicionRetorno=10;
	itemA->argumentos = list_create();
	itemA->identificadores = dictionary_create();
	itemA->valorRetorno=var;
	stack_push(stackA,itemA);
	t_stack_item* itemB = malloc(sizeof(t_stack_item));
	itemB->posicion=2;
	itemB->posicionRetorno=10;
	itemB->argumentos = list_create();
	itemB->identificadores = dictionary_create();
	t_variable* arg=malloc(sizeof(t_variable));
	arg->pagina=11;
	arg->offset=12;
	arg->size=13;
	list_add(itemB->argumentos,arg);
	itemB->valorRetorno=var;
	stack_push(stackA,itemB);
	char* serial = malloc(bytes_stack(stackA));
	serializar_stack(serial,stackA);
	imprimir_serializacion(serial,bytes_stack(stackA));
	deserializar_stack(stackB,serial);
	CU_ASSERT_EQUAL(stack_size(stackA),stack_size(stackB));
	CU_ASSERT_EQUAL(((t_stack_item *)stack_get(stackB,0))->posicion,1);
	CU_ASSERT_EQUAL(((t_stack_item *)stack_get(stackB,1))->posicionRetorno,10);
	CU_ASSERT_EQUAL(((t_variable*)(list_get(((t_stack_item *)stack_get(stackB,1))->argumentos,0)))->pagina,11);
	/*Liberar las estructuras internas*/
	stack_destroy(stackA);
	stack_destroy(stackB);
	free(serial);
}
void test_serializar_PCB(){
	//Creo el PCB
	t_PCB* pcb = malloc(sizeof(t_PCB));
	//Creo el pcb->SP
	pcb->SP = stack_create();
	t_variable var;
	var.pagina=1;
	var.offset=2;
	var.size=3;
	t_stack_item* itemA = malloc(sizeof(t_stack_item));
	itemA->posicion=1;
	itemA->posicionRetorno=10;
	itemA->argumentos = list_create();
	itemA->identificadores = dictionary_create();
	itemA->valorRetorno=var;
	stack_push(pcb->SP,itemA);
	t_stack_item* itemB = malloc(sizeof(t_stack_item));
	itemB->posicion=2;
	itemB->posicionRetorno=10;
	itemB->argumentos = list_create();
	itemB->identificadores = dictionary_create();
	t_variable* arg=malloc(sizeof(t_variable));
	arg->pagina=11;
	arg->offset=12;
	arg->size=13;
	list_add(itemB->argumentos,arg);

	itemB->valorRetorno=var;
	stack_push(pcb->SP,itemB);
	//Asigno lo demas
	pcb->PC=1;
	pcb->PID=10;
	pcb->cantidad_paginas=22;
	pcb->indice_codigo=list_create();
	t_sentencia* sen = malloc(sizeof(t_sentencia));
	sen->offset_inicio=88;
	sen->offset_fin=99;
	list_add(pcb->indice_codigo,sen);
	pcb->indice_etiquetas=dictionary_create();
	int* a = malloc(sizeof(int));
	*a = 267;
	dictionary_put(pcb->indice_etiquetas,"SALTO",a);
	char* serial = malloc(bytes_PCB(pcb));
	serializar_PCB(serial,pcb);
	imprimir_serializacion(serial,bytes_PCB(pcb));
	t_PCB* pcb2 = malloc(sizeof(t_PCB));
	deserializar_PCB(pcb2,serial);
	CU_ASSERT_EQUAL(pcb2->PID,10);
	CU_ASSERT_EQUAL(pcb->cantidad_paginas,22);
	CU_ASSERT_EQUAL(((t_stack_item*)stack_get(pcb2->SP,0))->posicion,1);
	CU_ASSERT_EQUAL(((t_stack_item*)stack_get(pcb2->SP,1))->posicion,2);
	CU_ASSERT_EQUAL(((t_sentencia*)list_get(pcb->indice_codigo,0))->offset_fin,99);
	CU_ASSERT_EQUAL(((*(int*)dictionary_get(pcb2->indice_etiquetas,"SALTO"))),267)
	/*Liberar las estructuras internas*/
	free(pcb);
	free(pcb2);
	free(serial);
}

