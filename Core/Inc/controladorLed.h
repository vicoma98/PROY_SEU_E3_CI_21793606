#ifndef controladorLed
#define controladorLed
extern int ledsEncendidos[8];
extern float temperatura;
extern float valorReal;
extern float temperaturaMax;
extern float temperaturaMin;
extern float temperaturaSalto;
void lecturaPonteciometroSetAlarma(void);
void encender_leds(void);
void ModoTestBasico(void);
#endif
