#ifndef controladorLed
#define controladorLed
extern int ledsEncendidos[8];
extern float temperatura;
extern float tempMax;
extern float tempMin;
extern float tempSalto;
extern float valorReal;
extern float luzMax;
extern float luzMin;
extern float luzSalto;
extern char estado;
void lecturaPonteciometroSetAlarma(void);
void encender_leds(void);
void ModoTestBasico(void);
void ntcReadAndmodify(void);
void ldrReadAndmodify(void);
void programaPrincipal(void);
#endif
