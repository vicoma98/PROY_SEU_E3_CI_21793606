#include "main.h"
#include "math.h"
#include <stdio.h>
static float NTC_B=3950.f;
static float NTC_R25=10000.f;
static float NTC_T25= 298.f;
static float NTC_RDIV= 10000.f;
int botonI=0;
int botonD=1;
int ntc=1;
int cambiar_modo=0;
int valor_anterior = 1;
int blinker_value = 1;
int llamadas = 0; // indicador de la cantidad de llamadas para parpadear
int reset_Buzzer=100000;//indicador de las iteraciones del programa de reseteo
int zumbar = 0; // indica si estapitando
int ledsEncendidos[8]={0,0,0,0,0,0,0,0};//display del estado de leds en funcion si deben estar encndidos o apagados
int led_level_buzzer;//ultimo led que debe estar parpadeando y el que marca la explosion
int ultimo_led; // ultimo led encendido fijo
extern ADC_HandleTypeDef hadc1;
ADC_ChannelConfTypeDef sConfig = {0};
extern UART_HandleTypeDef huart2;
float temperatura,valorReal;

//lista de leds
static const uint16_t ledPIN[]={GPIO_PIN_5,GPIO_PIN_0,GPIO_PIN_6,GPIO_PIN_3,GPIO_PIN_5,GPIO_PIN_8,GPIO_PIN_10,GPIO_PIN_4};
static const GPIO_TypeDef * ledGPIO[]={GPIOA,GPIOB,GPIOA,GPIOB,GPIOB,GPIOA,GPIOB,GPIOB};

void barridoLeds(void){
	int timeOn = 500000;
	//int timeOn = 100;
	for(int j = 0 ; j < 8 ; j++){
		for(int i = 0 ; i < timeOn ; i++){
				HAL_GPIO_WritePin(ledGPIO[j], ledPIN[j], 1);

		}
		HAL_GPIO_WritePin(ledGPIO[j], ledPIN[j], 0);
	}
	for(int j = 6 ; j >= 0 ; j--){
			for(int i = 0 ; i < timeOn ; i++){
					HAL_GPIO_WritePin(ledGPIO[j], ledPIN[j], 1);

			}
			HAL_GPIO_WritePin(ledGPIO[j], ledPIN[j], 0);
		}
}
void encender_leds(void){
	//enciende el resto de los leds en función de si son los que no parapadeán y necesitan estár encendidos
	for(int j = 0 ; j < 8 ; j++){
		if (j!=led_level_buzzer){
			HAL_GPIO_WritePin(ledGPIO[j], ledPIN[j], ledsEncendidos[j]);}

	}
	if (llamadas>=10000){
		if (led_level_buzzer>=8){
			led_level_buzzer=7;
		}

		HAL_GPIO_WritePin(ledGPIO[led_level_buzzer], ledPIN[led_level_buzzer], blinker_value);
		//cambia el valor de los leds para hacer que parpadén
		if(blinker_value==1){
			blinker_value=0;
		}else{
			blinker_value=1;
		}
		llamadas=0;
	}else{
		llamadas++;
	}
}

void setDisplay(int ultimo_led){
	for(int i = 0 ;i<8;i++){
			if ( ultimo_led>=i){
				ledsEncendidos[i]= 1;
			}else{
				ledsEncendidos[i]= 0;
			}
		}
}
void lecturaPonteciometroSetAlarma(void){

	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,10000);
	float valor=HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

		if(valor>20){
			led_level_buzzer=valor/500;
		}else{
			led_level_buzzer=-1;
		}
		if (led_level_buzzer>7){
			led_level_buzzer=7;
		}
		for(int i = 0 ;i<8;i++){
		if (led_level_buzzer>=0 && led_level_buzzer>=i){
			ledsEncendidos[i]= 1;
		}else{
			ledsEncendidos[i]= 0;
		}
	}
}

void ldrReadAndmodify(void){

	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,10000);
	float valorL=HAL_ADC_GetValue(&hadc1);
	valorReal = (1-(valorL/(float)0xfff))*100;//porcentaje de luminiscencia
	HAL_ADC_Stop(&hadc1);
	float stepLDR=12.5;
	ultimo_led=valorReal/stepLDR;//division para calcular el ultimo led encedido

	for(int i = 0 ;i<8;i++){
			if ( ultimo_led>=i){
				ledsEncendidos[i]= 1;
			}else{
				ledsEncendidos[i]= 0;
			}
		}
	//encender_leds();
}
void ntcReadAndmodify(void){
	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
			Error_Handler();
	}
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,10000);
	float B = 3950.f;
	float c = HAL_ADC_GetValue(&hadc1);
	float finalVal = c/(float)0xFFF * 3.3f;
	float resistencia = (3.3f * NTC_RDIV) / (3.3f - finalVal) - NTC_RDIV;
	temperatura = (NTC_B / (logf(resistencia / NTC_RDIV )+ NTC_B/NTC_T25)) - 273.f;
	float stepNTC=0.625;
	ultimo_led=(temperatura-25)/stepNTC;//division para calcular el ultimo led encedido
	for(int i = 0 ;i<8;i++){
		if ( ultimo_led>=i){
			ledsEncendidos[i]= 1;
		}else{
			ledsEncendidos[i]= 0;
		}
	}
	//encender_leds();


}
void buzzerStart(void){
	if(ultimo_led>=led_level_buzzer&&reset_Buzzer>100000){
		zumbar=1;
		HAL_GPIO_WritePin(Buzzer_GPIO_Port,Buzzer_Pin, 1);

	}else{
		reset_Buzzer++;
	}
}

void buzzerStop(void){
	if(zumbar==1&&botonD==0){
		HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, 0);
		botonD=1;
		reset_Buzzer=0;
		zumbar=0;
	}
}
void programaPrincipal(void){
	//leer pulsador y cambiar el modo de ldr a ntc o viciversa
	botonD= HAL_GPIO_ReadPin(Pulsador1_GPIO_Port,Pulsador1_Pin);
	botonI= HAL_GPIO_ReadPin(Pulsador2_GPIO_Port,Pulsador2_Pin);

	buzzerStart();
	buzzerStop();
	printf("la lectura de cambiar_modo es %d y la temperaruta es %00000.f y el sensor lumnico de %00000.f \r\n",ntc,temperatura,valorReal);
	printf("\033[H");
	if(ntc==0 && botonI==0){
		ntc=1;
	}else{
		if(ntc==1 && botonI==0){
			ntc=0;
		}
	}
	/*if( botonI==0){
		if(ntc==1 && botonI==0){
			ntc=0;

		}else{
			ntc=1;
		}
	}*/
	//enceder la lectura de los sensores
	if(ntc==0){
		ldrReadAndmodify();
	}else{
		ntcReadAndmodify();
	}
	encender_leds();
	lecturaPonteciometroSetAlarma();
}

void ModoTestBasico(void){
	botonD = HAL_GPIO_ReadPin(Pulsador1_GPIO_Port,Pulsador1_Pin);
	botonI = HAL_GPIO_ReadPin(Pulsador2_GPIO_Port,Pulsador2_Pin);
	ldrReadAndmodify();
	ntcReadAndmodify();
	printf("la lectura del boton derecho es %d la lectura del boton izquierdo es %d y la temperaruta es %00.f y el sensor lumnico de %00.f \r\n",botonD,botonI,temperatura,valorReal);
	printf("\033[H");
	barridoLeds();
}
