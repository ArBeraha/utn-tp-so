/*
 * log.h
 *
 *  Created on: 25/4/2016
 *      Author: utnso
 */

#ifndef OTROS_LOG_H_
#define OTROS_LOG_H_

t_log *activeLogger, *bgLogger;


void crearLogs(char* logname, char* procName);
void destruirLogs();

#endif /* OTROS_LOG_H_ */
