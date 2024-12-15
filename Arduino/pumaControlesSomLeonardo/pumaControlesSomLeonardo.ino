/* 
 * Controles de som Puma
 */

//#define HID_CUSTOM_LAYOUT
//#define LAYOUT_PORTUGUESE_BRAZILIAN

#include <Encoder.h>
//#include "Keyboard.h"
#include "HID-Project.h"

bool debug = false;
const int puma = 2;
const int aauto = 3;
const int fuel = 4;
const int ledCam = 6;
const int ledRemote = 5;
const int ledFuel = 7;
const int ledAuto = 8;
const int ledPuma = 9;
const int next = 19;
const int clk = 20;
const int dt = 21;

long timerLeds = 10000;

bool inicial = false;

Encoder myEnc(clk, dt);

void setup() {
  pinMode(puma, INPUT_PULLUP);
  pinMode(aauto, INPUT_PULLUP);
  pinMode(fuel, INPUT_PULLUP);
  pinMode(next,  INPUT_PULLUP);
  pinMode(ledCam, OUTPUT);
  pinMode(ledRemote, OUTPUT);
  pinMode(ledFuel, OUTPUT);
  pinMode(ledAuto, OUTPUT);
  pinMode(ledPuma, OUTPUT);
  if(debug){
    Serial.begin(9600);
    Serial.println("Basic Encoder Test:");
  } else {
    Keyboard.begin();
    Consumer.begin();
  }
  inicioLeds();
}

long oldPosition  = -999;

void loop() {
  //volume
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    if(newPosition>oldPosition+3){
      oldPosition = newPosition;
      if(debug){
          Serial.println("volume up");
        } else {
          Consumer.write(MEDIA_VOLUME_UP);
      }
    } 
    if(newPosition<oldPosition-3){
      oldPosition = newPosition;
      if(debug){
        Serial.println("volume down");
      } else {
        Consumer.write(MEDIA_VOLUME_DOWN);
      }
    }
  }
  //botão de próxima
  /*
  if(!digitalRead(next)) {
    proxMusica();
    if(debug){
      Serial.println("Next song");
    } else {
      Consumer.write(MEDIA_NEXT);
    }
    delay(250);
  }
  */
  //botão app puma
  if(!digitalRead(puma)) {
    if(debug){
      Serial.println("Puma App");
    } else {
      if(inicial){
        Keyboard.println("home");
        inicial = false;
      } else {
        inicial = true;
        Consumer.write(CONSUMER_BROWSER_HOME);
      }
    }
    ativaLed(ledPuma);
  }
  if(!digitalRead(aauto)) {
    inicial = false;
    if(debug){
      Serial.println("Android Auto");
    } else {
      Keyboard.println("auto");
    }
    ativaLed(ledAuto);
  }
  if(!digitalRead(fuel)) {
    inicial = false;
    if(debug){
      Serial.println("Abastecimento");
    } else {
      Keyboard.println("fuel");
    }
    ativaLed(ledFuel);
  }
  if(millis()>timerLeds){
    vaiVem();
    ligaTudo();
    timerLeds = millis() + (random(100) * 5000);
  }
}

void inicioLeds(){
  vaiVem();
  digitalWrite(ledPuma, HIGH);
  delay(250);
  digitalWrite(ledAuto, HIGH);
  delay(250);
  digitalWrite(ledFuel, HIGH);
  delay(250);
  digitalWrite(ledRemote, HIGH);
  delay(250);
  digitalWrite(ledCam, HIGH);
  delay(250);
}

void proxMusica(){
  apagaTudo();
  digitalWrite(ledPuma, HIGH);
  delay(50);
  digitalWrite(ledAuto, HIGH);
  delay(50);
  digitalWrite(ledFuel, HIGH);
  delay(50);
  digitalWrite(ledRemote, HIGH);
  delay(50);
  digitalWrite(ledCam, HIGH);
  delay(50);
}

void ativaLed(int numero){
  for(int i=0;i<6;i++){
    digitalWrite(numero,LOW);
    delay(150);
    digitalWrite(numero,HIGH);
    delay(150);
  }
}

void apagaTudo(){
  for(int i=5;i<10;i++){
    digitalWrite(i, LOW);
  }
}

void ligaTudo(){
  for(int i=5;i<10;i++){
    digitalWrite(i, HIGH);
  }
}

void vaiVem(){
  apagaTudo();
  /*
  for(int i=5;i<10;i++){
    digitalWrite(i,HIGH);
    delay(70);
    digitalWrite(i,LOW);
  }
  for(int i=9;i>4;i--){
    digitalWrite(i,HIGH);
    delay(70);
    digitalWrite(i,LOW);
  }
  */
  digitalWrite(ledPuma, HIGH);
  delay(70);
  digitalWrite(ledPuma, LOW);
  digitalWrite(ledAuto, HIGH);
  delay(70);
  digitalWrite(ledAuto, LOW);
  digitalWrite(ledFuel, HIGH);
  delay(70);
  digitalWrite(ledFuel, LOW);
  digitalWrite(ledRemote, HIGH);
  delay(70);
  digitalWrite(ledRemote, LOW);
  digitalWrite(ledCam, HIGH);
  delay(70);
  digitalWrite(ledCam, LOW);
  //voltando...
  digitalWrite(ledRemote, HIGH);
  delay(70);
  digitalWrite(ledRemote, LOW);
  digitalWrite(ledFuel, HIGH);
  delay(70);
  digitalWrite(ledFuel, LOW);
  digitalWrite(ledAuto, HIGH);
  delay(70);
  digitalWrite(ledAuto, LOW);
  digitalWrite(ledPuma, HIGH);
  delay(70);
  digitalWrite(ledPuma, LOW);
}