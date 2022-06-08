/*
 * utility.c
 *
 *  Created on: Apr 30, 2021
 *      Author: pperez
 */

#include "utility.h"
#include "main.h"
#include "cmsis_os.h"
#include <main.h>
#include "FreeRTOS.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdlib.h>
#include "semphr.h"
#include <string.h>
#include <task.h>
#include <math.h>

/////////////////////////////////////////////////////////////////////////////////////// BUFFER sin proteccion

uint32_t BUFF_is_empty(BUFF_BUFFER_t * buffer){
	return (buffer->n_elem==0);
}

uint32_t BUFF_is_full(BUFF_BUFFER_t * buffer){
	return (buffer->n_elem==buffer->size);
}

uint32_t BUFF_inserta(BUFF_BUFFER_t * buffer,BUFF_ITEM_t item){

	if (!buffer->isfull(buffer)){
		buffer->buff[buffer->cabeza]=item;
		buffer->cabeza=((buffer->cabeza)+1)%(buffer->size);
		buffer->n_elem++;
		return 1;
	} else return 0;
}

uint32_t BUFF_extrae(BUFF_BUFFER_t * buffer,BUFF_ITEM_t *item){

	if (!buffer->isempty(buffer)){
			*item=buffer->buff[buffer->cola];
			buffer->cola=(buffer->cola+1)%(buffer->size);
			buffer->n_elem--;

	} else return 0;

   return 1;
}

uint32_t BUFF_extrae_prot(BUFF_BUFFER_t * buffer,BUFF_ITEM_t *item)
{ //variable condicion
	int it;

	it=0;
	while (xSemaphoreTake(buffer->xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
	 // lock
	while(!BUFF_extrae(buffer,item)){
			 xSemaphoreGive(buffer->xSem);
	 		 vTaskDelay(100/portTICK_RATE_MS );
	 		 while (xSemaphoreTake(buffer->xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
	 		 it++;
	}
	xSemaphoreGive(buffer->xSem);
	return 1; // siempre tiene éxito
}
uint32_t BUFF_inserta_prot(BUFF_BUFFER_t * buffer,BUFF_ITEM_t item)
{ //variable condicion
	int it;

	it=0;
	while (xSemaphoreTake(buffer->xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
	 // lock
	while(!BUFF_inserta(buffer,item)){
	 		 xSemaphoreGive(buffer->xSem);
	 		 vTaskDelay(100/portTICK_RATE_MS );
	 		 while (xSemaphoreTake(buffer->xSem, 10000/portTICK_RATE_MS  ) != pdTRUE );
	 		 it++;
	}
	xSemaphoreGive(buffer->xSem);
	return 1; // siempre tiene éxito
}

uint32_t BUFF_inserta_cad(BUFF_BUFFER_t * buffer,BUFF_ITEM_t * cad,uint32_t nitems)
{ //variable condicion

	int it;
	uint32_t res;
	for (it=0;it<nitems;it++)
	{
		res=BUFF_inserta_prot(buffer,cad[it]);
		if (!res) return it;
	}
	return 1; // siempre tiene éxito
}


/*
typedef struct BUFFER_CIRCULAR_dummy {


	uint8_t    (*get)(uint8_t *);
	uint8_t    (*put)(uint8_t);
	uint32_t   (*BUFF_is_empty)(void);
	uint32_t   (*BUFF_is_full)(void);
	uint32_t   (*puts)(uint_t * cad,uint32_t length);
	uint32_t   (*gets)(uint_t * cad,uint32_t length);

	enum status{NOINIT,INIT};
	uint32_t 	cabeza;
	uint32_t	cola;
	uint8_t		buff[BUFF_MAX_ITEMS];
	uint32_t	n_elem;
} BUFF_BUFFER_t;
*/


// API
BUFF_BUFFER_t * bufferCreat(int size){
	BUFF_BUFFER_t * tmp;
	void * bff;

	tmp=malloc(sizeof(BUFF_BUFFER_t));
	if (tmp){ // enought for structure

		bff=malloc(sizeof(uint8_t)*size);
		if (bff){ //enought for buffer
			tmp->xSem=xSemaphoreCreateMutex();
			if (!tmp->xSem){
				free(bff);
				free(tmp);
				return NULL;
			};
            tmp->buff=bff;
			tmp->cabeza=0;
			tmp->cola=0;
			tmp->size=size;
			tmp->n_elem=0;
			tmp->get=(void*)BUFF_extrae_prot;
			tmp->put=(void*)BUFF_inserta_prot;
			tmp->puts=(void*)BUFF_inserta_cad;

			tmp->isfull=(void*)BUFF_is_full;
			tmp->isempty=(void*)BUFF_is_empty;
			return tmp;
		}
		else
			free(tmp);
	}
	return NULL;

}

