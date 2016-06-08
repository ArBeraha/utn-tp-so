/*
 * hilos.c
 *
 *  Created on: 7/6/2016
 *      Author: utnso
 */

#include "hilos.h"

void iniciarAtrrYMutexs(int cantidadDeMutex, ...) {
	pthread_attr_init(&detachedAttr);
	pthread_attr_setdetachstate(&detachedAttr, PTHREAD_CREATE_DETACHED);
	mutexs = list_create();
	va_list arguments;
	va_start(arguments, cantidadDeMutex);
	int i;
	for (i = 0; i < cantidadDeMutex; i++) {
		void* mutex = va_arg(arguments, void*);
		pthread_mutex_init(mutex, NULL);
		list_add(mutexs, mutex);
	}
	va_end(arguments);
}
int crearHilo(pthread_t * hilo, void *(*funcion)(void *)) {
	return pthread_create(hilo, &detachedAttr, (void*) funcion, NULL);
}
int crearHiloConParametro(pthread_t * hilo, void *(*funcion)(void *),
		void * parametro) {
	return pthread_create(hilo, &detachedAttr, (void*) funcion, parametro);
}
void finalizarAtrrYMutexs() {
	list_destroy_and_destroy_elements(mutexs, (void*) pthread_mutex_destroy);
	pthread_attr_destroy(&detachedAttr);
}
