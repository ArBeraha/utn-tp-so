/*
 * hilos.c
 *
 *  Created on: 7/6/2016
 *      Author: utnso
 */

#include "hilos.h"

void configHilos() {
	pthread_attr_init(&detachedAttr);
	pthread_attr_setdetachstate(&detachedAttr, PTHREAD_CREATE_DETACHED);
}

int crearHilo(pthread_t * hilo,   void *(*funcion) (void *)){
	return pthread_create(&hilo, &detachedAttr, (void*) funcion, NULL);
}

