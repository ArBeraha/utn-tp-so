/*
 * header.c
 *
 *  Created on: 23/4/2016
 *      Author: utnso
 */

#include "header.h"

char* headerToMSG(header_t header)
{
	return string_from_format("%c",header);
}
