#include "FreeRTOS.h"
#include <stdio.h>
#include "tareas_serie.h"
#include <task.h>
#include "cmsis_os.h"
#include "main.h"
#include "cJSON.h"
#include "utility.h"
#include <string.h>
#include <stm32f4xx_hal_dma.h>
#include "time.h"
#include "controladorLed.h"
void Task_Display( void *pvParameters );
void Task_DMA( void *pvParameters );
void Task_Send( void *pvParameters );
void Task_Receive( void *pvParameters );

#define buffer_DMA_size 2048
uint8_t buffer_DMA[buffer_DMA_size];
extern UART_HandleTypeDef huart1;
#define UART_ESP_AT_WIFI (&huart1)
#define UART_ESP8266 (&huart1)


//extern int ledsEncendidos[8];//display del estado de leds en funcion si deben estar encndidos o apagados
static const uint16_t ledPIN[]={GPIO_PIN_5,GPIO_PIN_0,GPIO_PIN_6,GPIO_PIN_3,GPIO_PIN_5,GPIO_PIN_8,GPIO_PIN_10,GPIO_PIN_4};
static const GPIO_TypeDef * ledGPIO[]={GPIOA,GPIOB,GPIOA,GPIOB,GPIOB,GPIOA,GPIOB,GPIOB};


char post_temp[]="POST /v1/queryContext HTTP/1.1\r\n"
		"Content-Type: application/json\r\n"
		"Accept: application/json\r\n"
		"Host: pperez-seu-or.disca.upv.es\r\n"
		"Content-Length: %d\r\n\r\n%s";
char post_temp_apli[]="POST /v1/updateContext HTTP/1.1\r\n"
		"Content-Type: application/json\r\n"
		"Accept: application/json\r\n"
		"Host: pperez-seu-or.disca.upv.es\r\n"
		"Content-Length: %d\r\n"
		"\r\n"
		"%s";
char identificador[]="Sensor_SEU_S6_VCM06";
char body []="{"
		"\"entities\":"
			"["
				"{\""
					"type\":\"Sensor\","
					"\"isPattern\":\"false\","
					"\"id\":\"Sensor_SEU_S6_VCM06\""
				"}"
			"]"
		"}";
char body_update []="{"
			"\"contextElements\":"
			"["
				"{"
				"\"type\": \"Sensor\","
				" \"isPattern\": \"false\","
				"\"id\": \"Sensor_SEU_S6_VCM06\","
				"\"attributes\": "
					"["
						"{"
							"\"name\": \"LEDS\","
							"\"type\": \"binary\","
							"\"value\": \"11111111\""
						"}"
					"]"
				"}"
			"],"
		"\"updateAction\": \"APPEND\""
		"}";
char body_update_todo []="{"
			"\"contextElements\":"
			"["
				"{"
				"\"type\": \"Sensor\","
				" \"isPattern\": \"false\","
				"\"id\": \"Sensor_SEU_S6_VCM06\","
				"\"attributes\": "
					"["
						"{"
							"\"name\": \"IntesnidadLuz\","
							"\"type\": \"floatArray\","
							"\"value\": \"%f,%f,%f,%f\""
						"},"
						"{"
							"\"name\": \"Temperatura\","
							"\"type\": \"floatArray\","
							"\"value\": \"%f,%f,%f,%f\""
						"},"
						"{"
							"\"name\": \"Alarma\","
							"\"type\": \"Boolean\","
							"\"value\": \"%c\""
						"}"
					"]"
				"}"
			"],"
		"\"updateAction\": \"APPEND\""
		"}";

char cadenafinalv2[1000];


extern BUFF_BUFFER_t * buff;
extern BUFF_BUFFER_t * buff_rx;

void serie_Init_FreeRTOS(void){

	BaseType_t res_task;

	//printf (PASCU_PRJ " at "__TIME__);
	fflush(0);

	buff=bufferCreat(128);
	if (!buff) return;

	buff_rx=bufferCreat(512);
	if (!buff_rx) return;

	res_task=xTaskCreate(Task_Display,"DISPLAY",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea Visualizador\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_DMA,"DMA",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
		if( res_task != pdPASS ){
				printf("PANIC: Error al crear Tarea Visualizador\r\n");
				fflush(NULL);
				while(1);
		}

	res_task=xTaskCreate(Task_Send,"ENVIO",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea Visualizador\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_Receive,"RECEIVE",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea Visualizador\r\n");
			fflush(NULL);
			while(1);
	}
}

char candenafinal[2000];
int funcion_conf(char * cadena,int len,int  osDelay_, int  osDelay_2){
	int vuelta=1;
	uint32_t res;
	res=HAL_UART_Transmit(UART_ESP_AT_WIFI,cadena,len,1000);
	HAL_UART_Receive_DMA(UART_ESP_AT_WIFI, buffer_DMA,buffer_DMA_size);
	osDelay(osDelay_);
	HAL_UART_DMAStop(UART_ESP_AT_WIFI);
	int buffer_ct1=buffer_DMA_size - HAL_DMA_getcounter(UART_ESP_AT_WIFI);
	int buffer_ct=0;
	while (buffer_ct<buffer_ct1)
		res=buff->put(buff,buffer_DMA[buffer_ct++]);
	osDelay(osDelay_2);

	return vuelta;
}

void json_querry(void){
	//tratado del json
	char *jsonp = strstr(buffer_DMA,"{");
	jsonp[strlen(jsonp)-2] = '\0';
	cJSON * contextResponses = cJSON_Parse(jsonp);
	cJSON * contextEl = cJSON_GetObjectItemCaseSensitive(contextResponses,"contextResponses");
	cJSON * array1 = cJSON_GetArrayItem(contextEl,0);
	cJSON * contextElement = cJSON_GetObjectItemCaseSensitive(array1,"contextElement");
	cJSON * atributes = cJSON_GetObjectItemCaseSensitive(contextElement,"attributes");
	cJSON * atributo =  cJSON_GetArrayItem(atributes,0);
	cJSON * values = cJSON_GetObjectItemCaseSensitive(atributo,"value");
	char* leds= values->valuestring;
	for(int i = 0; i<8;i++){
		if(leds[i]=='1'){
			ledsEncendidos[i]=1;
		}else{
			ledsEncendidos[i]=0;
		}
	}
	encender_leds();
}
void postfunc(char * nombreMaquina,char * ssid, char * passwd, char * puerto){
	uint32_t res;
	char cad[]="AT+CWJAP=\"%s\",\"%s\"\r\n";
	char cad1[]="AT+CWMODE=1\r\n";
	char cad2[]="AT+CIFSR\r\n";
	char cad3[]="AT+CIPSTART=\"TCP\",\"%s\",%s\r\n";
	sprintf(candenafinal,cad3,nombreMaquina,puerto);
	int r = funcion_conf(cad1,strlen(cad1),2000,1);
	sprintf(candenafinal,cad,ssid,passwd);
	int r2 = funcion_conf(candenafinal,strlen(candenafinal),2000,1);
	int r3 = funcion_conf(cad2,strlen(cad2),2000,1);
	sprintf(candenafinal,cad3,nombreMaquina,puerto);
	int r4 = funcion_conf(candenafinal,strlen(candenafinal),500,1000);

		//***post***


		char cad4[]="AT+CIPSEND=%d\r\n";
/* updatecontet
 		sprintf(candenafinal,post_temp_apli,strlen(body_update),&body_update);
		sprintf(cadenafinalv2,cad4,strlen(candenafinal));
		int r5 = funcion_conf(cadenafinalv2,strlen(cadenafinalv2),500,20);//send=de bytes
		int r6 = funcion_conf(candenafinal,strlen(candenafinal),500,20);//json peticion
*/
	sprintf(candenafinal,post_temp,strlen(body),&body);
	sprintf(cadenafinalv2,cad4,strlen(candenafinal));
	int r7 = funcion_conf(cadenafinalv2,strlen(cadenafinalv2),1000,2000);
	int r8 = funcion_conf(candenafinal,strlen(candenafinal),1000,2000);
	json_querry();

	while(1){
		osDelay(2000);
		int r7 = funcion_conf(cadenafinalv2,strlen(cadenafinalv2),500,500);
		int r8 = funcion_conf(candenafinal,strlen(candenafinal),500,500);

		json_querry();
	}




}


void Task_Send( void *pvParameters ){

	uint32_t it;
    uint32_t res;

	char cad[100];
	it=0;
	//boot_wifi("OPPOReno2","ilovematy");
	//entregable("worldclockapi.com","OPPOReno2","ilovematy","80");
	//postfunc("pperez-seu-or.disca.upv.es","routerSEU","00000000","10000");

	//postfunc("pperez2.disca.upv.es","OPPOReno2","ilovematy","10000");/*
	while(1){
		//ModoTestBasico();
		programaPrincipal();
		/*it++;
		sprintf(cad,"IT %d\r\n",(int)it);
		res=buff->puts(buff,(BUFF_ITEM_t *)cad,strlen(cad));
		vTaskDelay(10000/portTICK_RATE_MS );*/

	}//*/
}

void Task_Receive( void *pvParameters ){

	uint32_t it;
    uint32_t res;
    BUFF_ITEM_t  car;
#define buffer_length	128
    BUFF_ITEM_t  buffer[buffer_length];
    int buffer_ct,buffer_ct1;
    int crln_detect;

	it=0;


	while(1){
		it++;

		crln_detect=0;
		buffer_ct=0;

		while(crln_detect<2){
	    	res=buff->get(buff_rx,&car);
	    	buffer[buffer_ct++]=car;
	    	if (buffer_ct>1){

	    		if ((buffer[buffer_ct-2]=='\r')&&(buffer[buffer_ct-1]=='\n')) // \r\n detection end of line
					crln_detect=2;
				else
					if ((buffer_ct)==buffer_length)  // line out of limits --> error
						crln_detect=3;
	    	}

		}

		// prepare reception buffer from ESP
		HAL_UART_Receive_DMA(UART_ESP_AT_WIFI, buffer_DMA,buffer_DMA_size);
		// send line (command) to ESP
		res=HAL_UART_Transmit(UART_ESP_AT_WIFI,buffer,buffer_ct,1000);
		// wait a bit time
		osDelay(500);
		//stop reception probably all data are in dma buffer
		HAL_UART_DMAStop(UART_ESP_AT_WIFI);

		// send to console ESP answer.
		buffer_ct1=HAL_DMA_getcounter(UART_ESP_AT_WIFI);
		buffer_ct=0;
		while (buffer_ct<buffer_ct1)
			res=buff->put(buff,buffer_DMA[buffer_ct++]);
		// wait a bit time
		osDelay(1);
	}
}
