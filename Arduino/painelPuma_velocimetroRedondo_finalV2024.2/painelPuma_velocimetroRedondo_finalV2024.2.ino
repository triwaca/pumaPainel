/*******************************************************************************
 * Velocimetro da PUMA 2024.2
 * 
 * USAR ESP32C3 DEV
 * 
 * MAC ?
 * 
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/Org_01.h>
#include <Fonts/Picopixel.h>
#include <ArduinoJson.h>
#define I2C_SDA 4
#define I2C_SCL 5
#define TP_INT 0
#define TP_RST 1

//touch
#include "CST816D.h"
CST816D touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);
bool FingerNum;
uint8_t gesture;
uint16_t touchX, touchY;

bool modoVeloc = true; //diz se será um velocimetro ou um medidor de parâmetros
bool debug = false;
bool simulate = false;
bool redraw = false;
long redrawTimer = 15000;
long simTimer = 0;
int tempVel = 0;
int tempDir = random(359);
bool saindoAltaVeloc = false;
long watchdogData = 10000;

#include "iconePuma.c"
#define IMG_WIDTH 240
#define IMG_HEIGHT 78

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 10 /* CS */, 6 /* SCK */, 7 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);

String inputString = "";         // a String to hold incoming data
bool stringComplete = true;  // whether the string is complete

//memória para JSON
DynamicJsonDocument doc(400);

float gs_rad; //stores angle from where to start in radinats
float ge_rad; //stores angle where to stop in radinats

//example values for testing, use the values you wish to pass as argument while calling the function
byte cx=120; //x center
byte cy=120; //y center
byte radius=110; //radius
byte percent=80; //needle percent

int velValue = 0;
int tempValue = 0;
int tempMotorValue = 0;
bool sinalValue = false;
bool luzBaixaValue = false;
bool luzAltaValue = false;
int minutoValue = 0;
int horaValue = 0;
int combValue = 0;
float batValue = 0;
int direcValue = 0;
int limiteValue = 0;
int humValue = 0;
bool lockGpsValue = false;

String horaParaExibir = "";
int horaAtual = 0;
int minutoAtual = 0;
String dataParaExibir = "";

int veloc = 0;
int ultimaVeloc = 0;
int combustivel = 0;
bool lockGps = false;

int velExibida = 0;

int ultimaDir = 0;

int textoBussola = 0; //indica a direção na tela de 1 a 8


void setup() {
  // Init Display
  if (!gfx->begin()) {
    //Serial.println("gfx->begin() failed!");
  }

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->setRotation(1);
  gfx->fillScreen(BLACK);
  gfx->draw16bitRGBBitmap(0, 81, (const uint16_t *)iconePuma, IMG_WIDTH, IMG_HEIGHT);
  delay(500);
  gfx->fillScreen(BLACK);
  //desenhaBussola(0, false);
  if(modoVeloc){
    imprimeKmh(0);
    gfx->draw16bitRGBBitmap(83, 155, (const uint16_t *)miniPuma, 74, 75);
  }
  //gfx->setFont(&Org_01);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE, BLACK);
  /*
  for(int i=0;i<16;i++){
    gfx->print("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,;:()[]{}!?");
  }
  */
  Serial.begin(115200);
  inputString.reserve(200);
  touch.begin();
}

void loop() {
  FingerNum = touch.getTouch(&touchX, &touchY, &gesture);
  if (FingerNum){
    gfx->setCursor(70, 20);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setTextSize(2);
    gfx->println((String)touchX + " " + (String)touchY + " " + (String)gesture);
    if(touchX>180) modoVeloc = !modoVeloc;
    if(touchX<60) simulate = !simulate;
    delay(500);
    gfx->fillScreen(BLACK);
  }
  if (stringComplete) {
    if(debug){
      gfx->setTextColor(WHITE, BLACK);
      gfx->setCursor(95, 0);
      gfx->println("JSON");
      gfx->setCursor(70, 20);
      gfx->println("Vel:");
      gfx->setCursor(70, 40);
      gfx->println("Tmp:");
      gfx->setCursor(70, 60);
      gfx->println("Mot:");
      gfx->setCursor(70, 80);
      gfx->println("Sin:");
      gfx->setCursor(70, 100);
      gfx->println("Bai:");
      gfx->setCursor(70, 120);
      gfx->println("Alt:");
      gfx->setCursor(70, 140);
      gfx->println("Hor:");
      gfx->setCursor(70, 160);
      gfx->println("Min:");
      gfx->setCursor(70, 180);
      gfx->println("Cmb:");
      gfx->setCursor(70, 200);
      gfx->println("Bat:");
      gfx->setCursor(70, 220);
      gfx->println("Dir:");
      gfx->setCursor(160, 80);
      gfx->println("Hum:");
      gfx->setCursor(160, 100);
      gfx->println("Lim:");
      gfx->setCursor(160, 120);
      gfx->println("GPS:");
      gfx->setTextColor(YELLOW, BLACK);
      gfx->setCursor(120, 20);
      gfx->println("  ");
      gfx->setCursor(120, 20);
      gfx->println(velValue);
      gfx->setCursor(120, 40);
      gfx->println(tempValue);
      gfx->setCursor(120, 60);
      gfx->println(tempMotorValue);
      gfx->setCursor(120, 80);
      gfx->println(sinalValue);
      gfx->setCursor(120, 100);
      gfx->println(luzBaixaValue);
      gfx->setCursor(120, 120);
      gfx->println(luzAltaValue);
      gfx->setCursor(120, 140);
      gfx->println("  ");
      gfx->setCursor(120, 140);
      gfx->println(horaValue);
      gfx->setCursor(120, 160);
      gfx->println("  ");
      gfx->setCursor(120, 160);
      gfx->println(minutoValue);
      gfx->setCursor(120, 180);
      gfx->println(combValue);
      gfx->setCursor(120, 200);
      gfx->println(batValue);
      gfx->setCursor(120, 220);
      gfx->println("   ");
      gfx->setCursor(120, 220);
      gfx->println(direcValue);
      gfx->setCursor(210, 80);
      gfx->println(humValue);
      gfx->setCursor(210, 100);
      gfx->println(limiteValue);
      gfx->setCursor(210, 120);
      gfx->println(lockGpsValue);
    } else {
      if(modoVeloc){
        //gfx->fillScreen(BLACK);
        if(velValue>(limiteValue+2)){
          if(!saindoAltaVeloc){
            gfx->fillScreen(RED);
            gfx->draw16bitRGBBitmap(88, 160, (const uint16_t *)alerta, (const byte *)alerta_mask, 64, 64);
            saindoAltaVeloc = true;
          }
        } else {
          if(saindoAltaVeloc){ //rotina para apgar a tela vermelha mais rápido
            gfx->fillScreen(BLACK);
            saindoAltaVeloc = false;
            redraw = true;
          }
          if(direcValue!=ultimaDir){
            desenhaBussola(ultimaDir, true);
            if(velValue>3) desenhaBussola(direcValue, false);
            ultimaDir = direcValue;
          }
          //velocimetro(velValue);
          exibeHora();
          imprimeLuzes();
          exibeAlertas();
          mostraDirecao(ultimaDir);
        }
        velocimetroNumeros(velValue);
      } else {
        //modo mostrador
        for(int x = 0;x<=25*2;x++){
          desenhaMedidor(cx, cy, 120, 80, x*4, 0, 200,  0, (x<combValue*2));
        }
        desenhaMedidor(cx, cy, 120, 70, -1, 0, 50,  0, false);
        desenhaMedidor(cx, cy, 120, 70, 51, 0, 50,  0, false);
        gfx->setFont(&Org_01);
        gfx->setTextSize(4);
        gfx->setTextColor(WHITE, BLACK);
        gfx->setCursor(75, 30);
        gfx->println((String)tempValue);
        gfx->setTextSize(3);
        gfx->setCursor(124, 23);
        gfx->println("o");
        gfx->setCursor(142, 28);
        gfx->println("C");
        gfx->setTextColor(gfx->color565(128,128,128), BLACK);
        gfx->setCursor(94, 55);
        gfx->println((String)humValue+"%");
        gfx->setTextSize(2);
        gfx->setCursor(90, 70);
        gfx->println("HUMID");
        gfx->setCursor(40, 105);
        gfx->println("COMB");
        gfx->setCursor(145, 105);
        gfx->println("MOTOR");
        gfx->setCursor(82, 225);
        gfx->println("BATERIA");
        gfx->setTextSize(4);
        if(tempMotorValue<80){
          gfx->setTextColor(WHITE, BLACK);
        } else {
          gfx->setTextColor(RED, BLACK);
        }
        gfx->setCursor(145, 130);
        gfx->println("   ");
        gfx->setCursor(145, 130);
        gfx->println((String)tempMotorValue);
        gfx->setTextSize(3);
        gfx->setCursor(194, 123);
        gfx->println("o");
        gfx->setTextSize(4);
        if(combValue<4){
          gfx->setTextColor(RED, BLACK);
        } else {
          gfx->setTextColor(WHITE, BLACK);
        }
        gfx->setCursor(32, 130);
        gfx->println("  L");
        gfx->setCursor(32, 130);
        if(combValue<10){
          gfx->println("0" + (String)combValue);
        } else {
          gfx->println((String)combValue);
        }
        if(batValue<12){
          gfx->setTextColor(RED, BLACK);
        } else {
          gfx->setTextColor(WHITE, BLACK);
        }
        gfx->setCursor(70, 205);
        gfx->println((String)batValue + "V");
        //medidor de temperatura do motor
        for(int x = 15;x<=65;x++){
          desenhaMedidor(cx, cy, 120, 80, x-15, 0, 50,  1, (x<(tempMotorValue/2)));
        }
        desenhaMedidor(cx, cy, 120, 70, -1, 0, 50,  1, false);
        desenhaMedidor(cx, cy, 120, 70, 51, 0, 50,  1, false);
      }
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  /*
  gfx->fillScreen(BLACK);
  desenhaBussola(233, false);
  velocimetro(tempVel);
  imprimeKmh(tempVel);
  tempVel++;
  if(tempVel>111){
    tempVel = 0;
    desenhaBussola(233, true);
  }
  delay(250);
  */
  if(simulate){
    if(simTimer<millis()){
      simTimer = millis()+250;
      tempVel = tempVel + (random(10)-3);
      tempDir = tempDir + (random(6)-2);
      if(tempVel<0)tempVel = 0;
      if(tempVel>53) tempVel = tempVel-random(10);
      decodeJSON("{\"vel\":"+ (String)tempVel + ",\"direc\":" + (String)tempDir + + ",\"limite\":60,\"hora\":12,\"min\":32, \"temp\":28, \"tempMotor\":" + (String)(random(5)+70) + ", \"comb\":"+ (String)(4 + (random(2)-1))+", \"bat\":12.4, \"humi\":57}");
      stringComplete = true;
    }
  }
  if(redrawTimer<millis()){
    redrawTimer = millis()+15000;
    redraw = true;
    if(modoVeloc){
      if(velValue>(limiteValue+2)){
        gfx->fillScreen(RED);
        gfx->draw16bitRGBBitmap(88, 160, (const uint16_t *)alerta, (const byte *)alerta_mask, 64, 64);
      } else {
        gfx->fillScreen(BLACK);
        gfx->draw16bitRGBBitmap(83, 155, (const uint16_t *)miniPuma, 74, 75);
      }
      imprimeKmh(velValue);
    } else {
      gfx->fillScreen(BLACK);
    }
    if(watchdogData<millis()&&(!simulate)){
      gfx->setTextSize(4);
      gfx->setCursor(13, 100);
      gfx->setTextColor(RED, BLACK);
      gfx->println("SEM DADOS");
    }
  }
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      watchdogData = millis()+10000;
      decodeJSON(inputString);
    }
  }
}

void decodeJSON(String dados){
  DeserializationError error = deserializeJson(doc, dados);
  if (error) {
    gfx->setTextColor(RED, BLACK);
    gfx->setCursor(70, 20);
    gfx->println("Erro JSON!");
    gfx->println(error.f_str());
    return;
  }
  velValue = doc["vel"];
  tempValue = doc["temp"];
  tempMotorValue = doc["tempMotor"];
  sinalValue = doc["sinal"];
  luzBaixaValue = doc["baixa"];
  luzAltaValue = doc["alta"];
  minutoAtual = doc["min"];
  horaAtual = doc["hora"];
  combValue = doc["comb"];
  batValue = doc["bat"];
  direcValue = doc["direc"];
  humValue = doc["humi"];
  lockGpsValue = doc["gps"];
  limiteValue = doc["limite"];
}

void velocimetro(int velocidade){
  if(redraw){
    redraw = false;
    for(int x=0;x<=velocidade;x++){
      desenhaMedidor(cx,cy,100,95,x,0,130,3,true);
    }
  } else {
    if(velExibida<velocidade){
      for(int x=velExibida;x<=velocidade;x++){
        desenhaMedidor(cx,cy,100,95,x,0,130,3,true);
      }
    } else {
      for(int x=velocidade;x<=velExibida;x++){
        desenhaMedidor(cx,cy,100,95,x,0,130,3,false);
      }
    }
  }
}

void velocimetroNumeros(int velocidade){
  if(velExibida!=velocidade)imprimeKmh(velocidade);
  velExibida = velocidade;
}

void mostraDirecao(int heading){
  gfx->fillRect(0, 0, 256, 36, WHITE);
  gfx->setFont();
  gfx->setTextColor(BLACK, WHITE);
  switch ((heading+22)/45){
    case 0:
      gfx->setTextSize(3);
      gfx->setCursor(74, 12);
      gfx->print("Norte");
      break;
    case 1:
      gfx->setTextSize(2);
      gfx->setCursor(72, 18);
      gfx->print("Nordeste");
      break;
    case 2:
      gfx->setTextSize(3);
      gfx->setCursor(74, 12);
      gfx->print("Leste");
      break;
    case 3:
      gfx->setTextSize(2);
      gfx->setCursor(80, 18);
      gfx->print("Sudeste");
      break;
    case 4:
      gfx->setTextSize(3);
      gfx->setCursor(94, 12);
      gfx->print("Sul"); 
      break;
    case 5:
      gfx->setTextSize(2);
      gfx->setCursor(73, 18);
      gfx->print("Sudoeste"); 
      break;
    case 6:
      gfx->setTextSize(3);
      gfx->setCursor(74, 12);
      gfx->print("Oeste");
      break;
    case 7:
      gfx->setTextSize(2);
      gfx->setCursor(72, 18);
      gfx->print("Noroeste");
      break;
    case 8:
      gfx->setTextSize(3);
      gfx->setCursor(74, 12);
      gfx->print("Norte");
      break;
    default:
      gfx->setTextSize(2);
      gfx->setCursor(74, 12);
      gfx->print("--------");
      break;
  }    
}

void desenhaBussola(int heading, bool apagar){
  int limiteTelaSup = 35;
  int minVal = 0;
  int maxVal = 180;
  int p = 80;
  int r=120;
  gfx->setTextSize(2);
  heading = 360-heading;
  int n1=(r/100.00)*p; // calculate needle percent lenght
  int n2=(r/100.00)*105; // calculate needle percent lenght
  float gs_rad=0; //-90 degrees in radiant
  float ge_rad=3.142;
  float i=((heading-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  int xp = cx+(sin(i) * n1);
  int yp = cy-(cos(i) * n1);
  int xq = cx+(sin(i) * n2);
  int yq = cy-(cos(i) * n2);
  //gfx->drawLine(xp,yp,xq,yq, RED);
  if(yq>limiteTelaSup){
    gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
    gfx->setFont();
    if(apagar){
      gfx->setTextColor(BLACK);
    } else {
      gfx->setTextColor(RED);
    }
    gfx->println("N");
  }
  i=((heading+90-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  xp = cx+(sin(i) * n1);
  yp = cy-(cos(i) * n1);
  xq = cx+(sin(i) * n2);
  yq = cy-(cos(i) * n2);
  if(yq>40){
    gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
    if(apagar){
      gfx->setTextColor(BLACK);
    } else {
      gfx->setTextColor(WHITE);
    }
    gfx->println("L");
  }
  i=((heading+180-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  xp = cx+(sin(i) * n1);
  yp = cy-(cos(i) * n1);
  xq = cx+(sin(i) * n2);
  yq = cy-(cos(i) * n2);
  if(yq>limiteTelaSup){
    gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
    if(apagar){
      gfx->setTextColor(BLACK);
    } else {
      gfx->setTextColor(WHITE);
    }
    gfx->println("S");
  }
  i=((heading+270-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  xp = cx+(sin(i) * n1);
  yp = cy-(cos(i) * n1);
  xq = cx+(sin(i) * n2);
  yq = cy-(cos(i) * n2);
  if(yq>40){
    gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
    if(apagar){
      gfx->setTextColor(BLACK);
    } else {
      gfx->setTextColor(WHITE);
    }
    gfx->println("O");
  }
  //linhas
  n1=(r/100.00)*90; // calculate needle percent lenght
  n2=(r/100.00)*105; // calculate needle percent lenght
  for(int j=heading+15;j<heading+90;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(yq>limiteTelaSup){
      if(apagar){
        gfx->drawLine(xp,yp,xq,yq, BLACK);
      } else {
        if(j-heading==45){
          gfx->drawLine(xp,yp,xq,yq, WHITE);
        } else {
          gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
        }
      }
    }
  }
  for(int j=heading+105;j<heading+180;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(yq>limiteTelaSup){
      if(apagar){
        gfx->drawLine(xp,yp,xq,yq, BLACK);
      } else {
        if(j-heading==135){
          gfx->drawLine(xp,yp,xq,yq, WHITE);
        } else {
          gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
        }
      }
    }
  }
  for(int j=heading+195;j<heading+270;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(yq>limiteTelaSup){
      if(apagar){
        gfx->drawLine(xp,yp,xq,yq, BLACK);
      } else {
        if(j-heading==225){
          gfx->drawLine(xp,yp,xq,yq, WHITE);
        } else {
          gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
        }
      }
    }
  }
  for(int j=heading+285;j<heading+360;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(yq>limiteTelaSup){
      if(apagar){
        gfx->drawLine(xp,yp,xq,yq, BLACK);
      } else {
        if(j-heading==315){
          gfx->drawLine(xp,yp,xq,yq, WHITE);
        } else {
          gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
        }
      }
    }
  }
}

void desenhaMedidor(int x, byte y, byte r, byte p, int v, int minVal, int maxVal,  byte t, bool cor) {
  int n1=(r/100.00)*p; // calculate needle percent lenght
  int n2=(r/100.00)*110; // calculate needle percent lenght
  switch (t){
    case 0: { //left quarter
          /*
          float gs_rad=-1.572; //-90 degrees in radiant
          float ge_rad=0;/*
          float gs_rad=-1.572; //-90 degrees in radiant
          float ge_rad=0;
          */
          /*
          float gs_rad=-2.358; //-90 degrees in radiant
          float ge_rad=-0.786;
          float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
          int xp = x+(sin(i) * n1);
          */
          float gs_rad=-2.358; //-90 degrees in radiant
          float ge_rad=-0.786;
          float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
          int xp = x+(sin(i) * n1);
          int yp = y-(cos(i) * n1);
          int xq = x+(sin(i) * n2);
          int yq = y-(cos(i) * n2);
          if(cor){
            if(combValue<4){
              gfx->drawLine(xp,yp,xq,yq, RED);
            } else {
              gfx->drawLine(xp,yp,xq,yq, GREEN);
            }
          } else {
            gfx->drawLine(xp,yp,xq,yq, BLACK);
          }
        }
        break;
    case 1: { //right quarter, needle anticlockwise
  
          float gs_rad=-0.786;
          float ge_rad=0.786; //90 degrees in radiant
          float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
          int xp = x+(cos(i) * n1);
          int yp = y-(sin(i) * n1);
          int xq = x+(cos(i) * n2);
          int yq = y-(sin(i) * n2);
          if(cor){
            if(tempMotorValue<60){
              gfx->drawLine(xp,yp,xq,yq, GREEN);
            } else if(tempMotorValue<80){
              gfx->drawLine(xp,yp,xq,yq, YELLOW);
            } else {
              gfx->drawLine(xp,yp,xq,yq, RED);
            }
          } else {
            gfx->drawLine(xp,yp,xq,yq, BLACK);
          }
        }
        break; 
    case 2: { //upper half
          float gs_rad=-3.142;
          float ge_rad=1.572;
          float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
          int xp = x+(sin(i) * n1);
          int yp = y-(cos(i) * n1);
          int xq = x+(sin(i) * n2);
          int yq = y-(cos(i) * n2);
          gfx->drawLine(xp,yp,xq,yq, WHITE);
        }
        break;
    case 3: { //three quarter starting at -180
          float gs_rad=-3.142;
          float ge_rad=1.572;
          float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
          int xp = x+(sin(i) * n1);
          int yp = y-(cos(i) * n1);
          int xq = x+(sin(i) * n2);
          int yq = y-(cos(i) * n2);
          if(cor){
            if(velExibida<52){
              gfx->drawLine(xp,yp,xq,yq, WHITE); 
            } else if(velExibida<=80){
              gfx->drawLine(xp,yp,xq,yq, YELLOW); 
            } else {
              gfx->drawLine(xp,yp,xq,yq, RED); 
            }
          } else {
            gfx->drawLine(xp,yp,xq,yq, BLACK); 
          }
        } 
        break;
    case 4: { //three quarter starting at -90
        float gs_rad=-1.572;
        float ge_rad=3.141;
        float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
        int xp = x+(sin(i) * n1);
        int yp = y-(cos(i) * n1);
        gfx->drawLine(x,y,xp,yp, WHITE);
      }
      break;
  }  
}

void exibeHora(){
  if((horaAtual==0)and(minutoAtual==0)){
    //sem hora
  } else {
    horaParaExibir = (String)horaAtual + ":";
    if(minutoAtual<10) horaParaExibir = horaParaExibir + "0";
    horaParaExibir = horaParaExibir + (String)minutoAtual;
    gfx->fillRect(70, 42, 10, 28, BLACK);
    gfx->setFont(&Org_01);
    gfx->setTextSize(4);
    gfx->setTextColor(WHITE, BLACK);
    int posicHora = 240;
    int dezena = horaAtual/10;
    int unidade = horaAtual - (dezena*10);
    if(dezena==1){
      posicHora = posicHora - 6;
    } else {
      if(dezena>0){
        posicHora = posicHora - 18;
      }
    }
    if(unidade==1){
      posicHora = posicHora - 6;
    } else {
      posicHora = posicHora - 18;
    }
    dezena = minutoAtual/10;
    unidade = minutoAtual - (dezena*10);
    if(dezena==1){
      posicHora = posicHora - 6;
    } else {
      posicHora = posicHora - 18;
    }
    if(unidade==1){
      posicHora = posicHora - 6;
    } else {
      posicHora = posicHora - 18;
    }
    gfx->setCursor((posicHora/2)-12, 60);
    gfx->print(horaParaExibir);
  }
}

void exibeAlertas(){
  if(sinalValue){
        gfx->draw16bitRGBBitmap(38, 160, (const uint16_t *)sinal, 38, 20);
  }
  if((combValue<4)||(tempValue>80)||(batValue<12)){
    gfx->draw16bitRGBBitmap(88, 160, (const uint16_t *)alerta, (const byte *)alerta_mask, 64, 64);
  } else {
    gfx->draw16bitRGBBitmap(83, 155, (const uint16_t *)miniPuma, 74, 75);
  }
}

void imprimeLuzes(){
  if(luzBaixaValue){
      gfx->draw16bitRGBBitmap(171, 163, (const uint16_t *)baixa, 38, 38);
    } else {
      if(luzAltaValue){
        gfx->draw16bitRGBBitmap(171, 163, (const uint16_t *)alta, 38, 38);
      } else {
        gfx->fillRect(171, 163, 38, 38, BLACK);
      } 
    }
}

void imprimeKmh(int valor){
   int unid_x = 0;
   int dez_x = 0;
   int cent_x = 0;
   if(valor>99){
     unid_x = 145;
     dez_x = 56;
     cent_x = -20;
   } else {
     unid_x = 115;
     dez_x = 26;
     cent_x = -70;
   }
  int centena = valor/100;
  int decimal = valor/10 - (centena * 10);
  int unid = valor - (decimal * 10) - (centena * 100);
  if(((veloc>99)and(velExibida<=99))or((veloc<=99)and(velExibida>99))){
    gfx->fillRect(0, 75, 240, 90, BLACK);
  }
  ultimaVeloc = veloc;
  switch(unid){
    case 0:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n0o, 89, 76);
      break;
    case 1:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n1o, 89, 76);
      break;
    case 2:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n2o, 89, 76);
      break;
    case 3:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n3o, 89, 76);
      break;
    case 4:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n4o, 89, 76);
      break;
    case 5:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n5o, 89, 76);
      break;
    case 6:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n6o, 89, 76);
      break;
    case 7:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n7o, 89, 76);
      break;
    case 8:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n8o, 89, 76);
      break;
    case 9:
      gfx->draw16bitRGBBitmap(unid_x, 78, (const uint16_t *)n9o, 89, 76);
      break;
  }
  switch(decimal){
    case 0:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n0o, 89, 76);
      break;
    case 1:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n1o, 89, 76);
      break;
    case 2:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n2o, 89, 76);
      break;
    case 3:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n3o, 89, 76);
      break;
    case 4:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n4o, 89, 76);
      break;
    case 5:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n5o, 89, 76);
      break;
    case 6:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n6o, 89, 76);
      break;
    case 7:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n7o, 89, 76);
      break;
    case 8:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n8o, 89, 76);
      break;
    case 9:
      gfx->draw16bitRGBBitmap(dez_x, 78, (const uint16_t *)n9o, 89, 76);
      break; 
  }
  switch(centena){
    case 0:
      //gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n0o, 89, 76);
      break;
    case 1:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n1o, 89, 76);
      break;
    case 2:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n2o, 89, 76);
      break;
    case 3:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n3o, 89, 76);
      break;
    case 4:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n4o, 89, 76);
      break;
    case 5:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n5o, 89, 76);
      break;
    case 6:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n6o, 89, 76);
      break;
    case 7:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n7o, 89, 76);
      break;
    case 8:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n8o, 89, 76);
      break;
    case 9:
      gfx->draw16bitRGBBitmap(cent_x, 78, (const uint16_t *)n9o, 89, 76);
      break; 
  }
}
