/*
 * log.h
 *
 *  Created on: 25/4/2016
 *      Author: utnso
 */

#ifndef OTROS_LOG_H_
#define OTROS_LOG_H_
#include <stdio.h>
#include <stdlib.h>

#define LOG_PRINT_NOTHING -1
#define LOG_PRINT_DEFAULT 0
#define LOG_PRINT_ALL 1


t_log *activeLogger, *bgLogger;


void crearLogs(char* logname, char* procName, int logLevel);
void destruirLogs();
void reactivarLogs();
void destruirLogs();

#endif /* OTROS_LOG_H_ */
