/*
 * algoritmosReemplazo.c
 *
 *  Created on: 19/6/2016
 *      Author: utnso
 */

#include "umc.h"

int sacarConClock(int pid){

	pthread_mutex_lock(&lock_accesoTabla);
	pthread_mutex_lock(&lock_accesoUltimaPos);

	tabla_t* tabla = malloc(sizeof(tabla_t));
	tabla = buscarTabla(pid); //TABLA CON ID A REEMPLAZAR
	int cantidadPaginas = list_size((t_list*)tabla->listaPaginas);
	int posAReemplazar;
	tablaPagina_t* puntero;

	//Primera vuelta, doy segunda oportunidad

	for(posAReemplazar=buscarUltimaPosSacada(pid); posAReemplazar<cantidadPaginas; posAReemplazar++){
			puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
			if(puntero->bitUso==0 && puntero->bitPresencia==1){
				cambiarUltimaPosicion(pid,posAReemplazar);
				pthread_mutex_unlock(&lock_accesoUltimaPos);
				pthread_mutex_unlock(&lock_accesoTabla);
				return posAReemplazar;
			}else{
				puntero->bitUso=0;
			}
		}

	//Segunda vuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}
	}

	pthread_mutex_unlock(&lock_accesoUltimaPos);
	pthread_mutex_unlock(&lock_accesoTabla);
	log_error(activeLogger, "[%d] Algo fallo en CLOCK",pid);
	return -1;
}

int sacarConModificado(int pid){

	pthread_mutex_lock(&lock_accesoTabla);
	pthread_mutex_lock(&lock_accesoUltimaPos);

	tabla_t* tabla = malloc(sizeof(tabla_t));
	tabla = buscarTabla(pid);
	int cantidadPaginas = list_size((t_list*)tabla->listaPaginas);
	int posAReemplazar;
	tablaPagina_t* puntero;

	//Primera vuelta, me fijo si hay alguno (0,0) sin modificar nada
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitModificacion==0 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitModificacion==1 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//PrimerVuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	//Segunda vuelta
	for(posAReemplazar=buscarUltimaPosSacada(pid);posAReemplazar<cantidadPaginas;posAReemplazar++){
		puntero = list_get((t_list*)tabla->listaPaginas,posAReemplazar%cantidadPaginas);
		if(puntero->bitUso==0 && puntero->bitModificacion==1 && puntero->bitPresencia==1){
			cambiarUltimaPosicion(pid,posAReemplazar);
			pthread_mutex_unlock(&lock_accesoUltimaPos);
			pthread_mutex_unlock(&lock_accesoTabla);
			return posAReemplazar;
		}else{
			puntero->bitUso=0;
		}
	}

	pthread_mutex_unlock(&lock_accesoUltimaPos);
	pthread_mutex_unlock(&lock_accesoTabla);
	log_error(activeLogger, "[%d] Algo fallo en CLOCK MODIFICADO",pid);

	return -1;
}
