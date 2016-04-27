/*
 * commonTypes.h
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#ifndef OTROS_COMMONTYPES_H_
#define OTROS_COMMONTYPES_H_

typedef int ansisop_var_t;



typedef struct t_PCB{
	int PID; // identificador único
	int PC;	 // Program Counter
	int SP;  // Posición del Stack
	// MAGIA DE PCB!
} t_PCB;
#endif /* OTROS_COMMONTYPES_H_ */
