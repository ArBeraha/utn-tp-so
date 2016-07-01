/*
 * auxiliaresUmc.c
 *
 *  Created on: 19/6/2016
 *      Author: utnso
 */

#include "umc.h"

int primerNumeroPaginaLibre(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* auxiliar = buscarTabla(pid);
	pthread_mutex_unlock(&lock_accesoTabla);
	return list_size((t_list*)auxiliar->listaPaginas);
}

void inicializarTlb(){
	pthread_mutex_lock(&lock_accesoTlb);
	tiempo=0;
	int i;
	for(i = 0; i<config.entradas_tlb; i++){
		tlb[i].pid=-1;
		tlb[i].pagina=-1;
		tlb[i].marcoUtilizado=-1;
		tlb[i].contadorTiempo=-1;
	}
	pthread_mutex_unlock(&lock_accesoTlb);
}

void flushTlbDePid(int pidViejo){
	pthread_mutex_lock(&lock_accesoTlb);
		int i;
		for(i = 0; i<config.entradas_tlb; i++){
			if(tlb[i].pid==pidViejo){
				tlb[i].pid=-1;
				tlb[i].pagina=-1;
				tlb[i].marcoUtilizado=-1;
				tlb[i].contadorTiempo=-1;
			}
		}
	pthread_mutex_unlock(&lock_accesoTlb);
}


int buscarUltimaPosSacada(int pidParam){
	int i;
	int size = list_size(listaUltimaPosicionSacada);
	for(i=0;i<size;i++){
		ultimaSacada_t* entrada = list_get(listaUltimaPosicionSacada,i);
		if(entrada->pid == pidParam){
			return entrada->posicion;
		}
	}
	return -1;
}

void cambiarUltimaPosicion(int pidParam, int ultima){
	int i;
	int size = list_size(listaUltimaPosicionSacada);
	for(i=0;i<size;i++){
		ultimaSacada_t* entrada = list_get(listaUltimaPosicionSacada,i);
		if(entrada->pid == pidParam){
			entrada->posicion=ultima;
		}
	}
}


int estaEnTlb(pedidoLectura_t pedido){
	pthread_mutex_lock(&lock_accesoTlb);
	int i;

	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			pthread_mutex_unlock(&lock_accesoTlb);
			return 1;
		}
	}
	pthread_mutex_unlock(&lock_accesoTlb);
	return 0;
}

int buscarEnTlb(pedidoLectura_t pedido){ //Repito codigo, i know, pero esta soluc no funciona para las dos, porque si se encuentra el pedido en tlb[0] y retornas 'i', "no estaria en tlb" cuando si
	pthread_mutex_lock(&lock_accesoTlb);
	int i;
	for(i=0;i<config.entradas_tlb; i++){
		if(tlb[i].pid==pedido.pid && tlb[i].pagina==pedido.paginaRequerida){
			tlb[i].contadorTiempo = tiempo++;
			pthread_mutex_unlock(&lock_accesoTlb);
			return i;
		}
	}
	pthread_mutex_unlock(&lock_accesoTlb);
	return 0;
}

int buscarPosicionTabla(int pidBusca){
	int size = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<size;i++){
		tabla_t* tabla;
		tabla = list_get(listaTablasPaginas,i);
		if(tabla->pid==pidBusca) return i;
	}
	return -1; //No existe, avisa que hay que agregarla para reservarPagina
}

tabla_t* buscarTabla(int pidBusca){
	int size = list_size(listaTablasPaginas);
	int i;

	for(i=0;i<size;i++){
		tabla_t* tabla;
		tabla = list_get(listaTablasPaginas,i);
		if(tabla->pid==pidBusca) return tabla;
	}
	return NULL; //No existe, avisa que hay que agregarla para reservarPagina
}

int existePidEnListadeTablas(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* tabla = malloc(sizeof(tabla_t));

	tabla=buscarTabla(pid);

	if(list_size(tabla->listaPaginas)==0){
		pthread_mutex_unlock(&lock_accesoTabla);
		return 0;
	}else{
		pthread_mutex_unlock(&lock_accesoTabla);
		return 1;
	}
}

int existePaginaBuscadaEnTabla(int pag, tabla_t* tablaPaginaBuscada){
	tablaPagina_t* tabla = malloc(sizeof(tablaPagina_t));

	pthread_mutex_lock(&lock_accesoTabla);

	tabla=list_get((t_list*)tablaPaginaBuscada->listaPaginas, pag);

	if(tabla){
		pthread_mutex_unlock(&lock_accesoTabla);
		return 1;
	}else{
		pthread_mutex_unlock(&lock_accesoTabla);
		return 0;
	}
}

int buscarPrimerMarcoLibre(){
	int i;
	pthread_mutex_lock(&lock_accesoMarcosOcupados);
	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			pthread_mutex_unlock(&lock_accesoMarcosOcupados);
			return i;
		}
	}
	pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	return -1;
}


int cantidadMarcosLibres(){
	int i;
	int c=0;

	pthread_mutex_lock(&lock_accesoMarcosOcupados);
	for(i=0;i<config.cantidad_marcos;i++){
		if(vectorMarcosOcupados[i]==0){
			c++;
		}
	}
	pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	return c;
}

void reemplazarEntradaConLru(tablaPagina_t* pagina,int pidParam,int *pid,int *pag,int *marco){ //Y la agrega tmb...
	pthread_mutex_lock(&lock_accesoTlb);
	int menorTiempo= tlb[0].contadorTiempo;
	int posicionMenorTiempo = 0;
	int i;
	for(i=0;i<config.entradas_tlb;i++){
		if(tlb[i].contadorTiempo < menorTiempo){
			menorTiempo = tlb[i].contadorTiempo;
			posicionMenorTiempo = i;
		}
	}

	*pid = tlb[posicionMenorTiempo].pid;
	*pag = tlb[posicionMenorTiempo].pagina;
	*marco = tlb[posicionMenorTiempo].marcoUtilizado;

	tlb[posicionMenorTiempo].pid=pidParam;
	tlb[posicionMenorTiempo].pagina=pagina->nroPagina;
	tlb[posicionMenorTiempo].marcoUtilizado=pagina->marcoUtilizado;
	tlb[posicionMenorTiempo].contadorTiempo=tiempo++;

	pthread_mutex_unlock(&lock_accesoTlb);
}

void agregarATlb(tablaPagina_t* pagina,int pidParam){
	if(config.entradas_tlb){
		pedidoLectura_t pedido;
		pedido.pid = pidParam;
		pedido.paginaRequerida = pagina->nroPagina;
		pedido.offset=0;
		pedido.cantBytes=0;

		if(estaEnTlb(pedido)==0){
			int i;
			pthread_mutex_lock(&lock_accesoTlb);
			for(i=0;i<config.entradas_tlb;i++){
				if(tlb[i].pid==-1){
					//Se encontro un espacio libre en la tlb, se va a guardar ahi
					tlb[i].pagina = pagina->nroPagina;
					tlb[i].marcoUtilizado = pagina->marcoUtilizado;
					tlb[i].pid = pidParam;
					tlb[i].contadorTiempo = tiempo++;

					pthread_mutex_unlock(&lock_accesoTlb);
					log_info(activeLogger, "[%d][T] Agregado a TLB [Pagina,Marco] = [%d,%d]",pidParam,pedido.paginaRequerida,pagina->marcoUtilizado);
					return;
				}
			}
			pthread_mutex_unlock(&lock_accesoTlb);
			int pid=0,pag=0, marco=0;
			reemplazarEntradaConLru(pagina,pidParam,&pid,&pag,&marco);
			log_info(activeLogger, "[%d][T] Agregado a TLB [Pagina,Marco] = [%d,%d]. Habiendo reemplazado: [%d,%d,%d] [Pag,Marco,Pid] ",pidParam,pedido.paginaRequerida,pagina->marcoUtilizado,pag,marco,pid);
		}
	}
	else{
		log_info(activeLogger, "La TLB NO esta habilitada, por lo tanto no se agrego la entrada");
	}
}

void sacarPosDeTlb(int pos){
	tlb[pos].pid=-1;
	tlb[pos].pagina=-1;
	tlb[pos].marcoUtilizado=-1;
	tlb[pos].contadorTiempo=-1;
}

void sacarDeMemoria(tablaPagina_t* pagina, int pid){
	pthread_mutex_lock(&lock_accesoMemoria);

	memset(memoria+(pagina->marcoUtilizado * config.tamanio_marco),'\0',config.tamanio_marco);

	pthread_mutex_lock(&lock_accesoMarcosOcupados);

	vectorMarcosOcupados[pagina->marcoUtilizado]=0;

	pthread_mutex_unlock(&lock_accesoMarcosOcupados);

	pagina->marcoUtilizado=-1;

	pthread_mutex_unlock(&lock_accesoMemoria);

	pedidoLectura_t pedidoFalso;
	pedidoFalso.pid = pid;
	pedidoFalso.paginaRequerida = pagina->nroPagina;

	if(estaEnTlb(pedidoFalso)){
		int pos = buscarEnTlb(pedidoFalso);
		sacarPosDeTlb(pos);
	}
}

int cantPaginasDePid(int pid){
	tabla_t* tablaPaginaAReemplazar = buscarTabla(pid);
	return list_size((t_list*)tablaPaginaAReemplazar->listaPaginas);
}

int cantPaginasEnMemoriaDePid(int pid){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* tablaPaginaAReemplazar = buscarTabla(pid);
	int cantidadPaginas = list_size((t_list*)tablaPaginaAReemplazar->listaPaginas);

	int i;
	int contador=0;

	for(i=0;i<cantidadPaginas;i++){
		tablaPagina_t* unaPagina;
		unaPagina = list_get((t_list*)tablaPaginaAReemplazar->listaPaginas,i);
		if(unaPagina->bitPresencia==1){
			contador++;
		}
	}
	pthread_mutex_unlock(&lock_accesoTabla);
	return contador;
}

int paginasQueOcupa(int tamanio){
	return (tamanio + config.tamanio_marco - 1) / config.tamanio_marco;
}

void ponerBitModif1(int pid,int pag){
	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* tabla = buscarTabla(pid);
	tablaPagina_t* pagina = list_get((t_list*)tabla->listaPaginas,pag);
	pagina->bitModificacion=1;
	pthread_mutex_unlock(&lock_accesoTabla);
}

void sacarMarcosOcupados(int idPrograma){

	pthread_mutex_lock(&lock_accesoTabla);
	tabla_t* auxiliar = buscarTabla(idPrograma);

	tablaPagina_t* tabla = malloc(sizeof(tablaPagina_t));
	int i;
	int size = list_size((t_list*)auxiliar->listaPaginas);
	for(i=0;i<size;i++){
		tabla = list_get((t_list*)auxiliar->listaPaginas,i);

		pthread_mutex_lock(&lock_accesoMarcosOcupados);
		vectorMarcosOcupados[tabla->marcoUtilizado] = 0;
		pthread_mutex_unlock(&lock_accesoMarcosOcupados);
	}
	pthread_mutex_unlock(&lock_accesoTabla);
}

void imprimirRegionMemoriaStack(char* region, int size){
	int i=0;
	printf("(REGION STACK):");
	int valor;
	while(i<size){
		valor = char4ToInt(region+i);
		printf("%d",valor);
		i=i+4;
	}
	printf("\n");
}

void imprimirRegionMemoriaCodigo(char* region, int size){
    int i;
    printf("(REGION CODIGO):");
    char* paraLog = malloc(size+1);
    for(i=0;i<size;i++){
        if (region[i]>=32 || region[i]=='\n')
            printf("%c",region[i]);
            paraLog[i]=region[i];
    }
    paraLog[size]='\0';
    log_info(activeLogger,"%s",paraLog);
    free(paraLog);
}

void mostrarTlb(){
	if(config.entradas_tlb){
		int i;
		printf("\n -- TLB -- \n");
		for(i=0;i<config.entradas_tlb;i++){
			tlb_t unaEntrada;
			unaEntrada = tlb[i];

			printf("Pos: %d Pid: %d Pag: %d Marco: %d Contador: %d \n",i,unaEntrada.pid,unaEntrada.pagina,unaEntrada.marcoUtilizado,unaEntrada.contadorTiempo);

		}
	}
}
