/*
 * Simulador de sinal de RPM para Arduino UNO
 * Envia um pulso em velocidades correspondentes ao RPM de um motor boxer
 * Cada giro do virabrequim é composto de 4 disparos das velas.
 * Um motor a 1200 RPM terá um pulso a cada 50 milissegundos.
 * Hardware:
 *   - Potenciômetro ligado na porta A2
 *   - Display 7 segmentos 8 caracteres MAX7219
          pin 12 DataIn 
          pin 11 CLK 
          pin 10 LOAD 
   Para analisar a saída do sinal no osciloscópio DSO138, basta:
   - Ligar medidor vermelho no pino 13;
   - Ligar medidor preto no pino GND;
   - Setar CPL em DC;
   - Setar SEN1 em 1V;
   - Setar SEN2 em X1;
   - Baixar onda no display até ficar visível.
 *
 */
#include "LedControl.h"

LedControl lc=LedControl(12,11,10,1);

const int sinalSaida = 13;
int valorRPM = 1200;
int mil = 0;
int cen = 0;
int dez = 0;
int uni = 0;
long timerScreen = 0;

void setup() {
  pinMode(sinalSaida, OUTPUT);
  Serial.begin(9600);
  Serial.println("RPM Simulador");
  lc.shutdown(0,false);
  lc.setIntensity(0,8);
  lc.clearDisplay(0);
}

void loop() {
  
  valorRPM = map(analogRead(A2),0,1023,750,4300);
  if(timerScreen<millis()){
    timerScreen = millis()+250;
    mil = valorRPM/1000;
    lc.setDigit(0,3,mil,false);
    cen = (valorRPM/100)-(mil * 10);
    lc.setDigit(0,2,cen,false);
    dez = (valorRPM/10)-(mil * 100)-(cen*10);
    lc.setDigit(0,1,dez,false);
    lc.setDigit(0,0,0,false);
    Serial.println(valorRPM);
  }
  
  digitalWrite(sinalSaida, HIGH);
  delay(1);
  digitalWrite(sinalSaida, LOW);
  delay((60000/(valorRPM*4)));
  
}
