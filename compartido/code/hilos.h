/*
 * hilos.h
 *
 *  Created on: 7/6/2016
 *      Author: utnso
 */

#ifndef CODE_HILOS_H_
#define CODE_HILOS_H_

#include <pthread.h>
#include <stdarg.h>
#include <commons/collections/list.h>

pthread_attr_t detachedAttr;
t_list* mutexs;

// Dejo un macro para no tener que estar rodeando todo con mutexs, pueden creer otro macro en base a este por cada mutex especifico
#define MUTEX(CONTENIDO,MUTX) \
	{pthread_mutex_lock(&MUTX); \
	CONTENIDO; \
	pthread_mutex_unlock(&MUTX);} \

void iniciarAtrrYMutexs(int cantidadDeMutexs, ...);
int crearHilo(pthread_t * hilo,   void *(*funcion) (void *));
int crearHiloConParametro(pthread_t * hilo,   void *(*funcion) (void *), void * parametro);
void finalizarAtrrYMutexs();

#endif /* CODE_HILOS_H_ */
