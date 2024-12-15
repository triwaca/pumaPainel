/*******************************************************************************
 * Start of Arduino_GFX setting
 *
 * Arduino_GFX try to find the settings depends on selected board in Arduino IDE
 * Or you can define the display dev kit not in the board list
 * Defalult pin list for non display dev kit:
 * Arduino Nano, Micro and more: CS:  9, DC:  8, RST:  7, BL:  6, SCK: 13, MOSI: 11, MISO: 12
 * ESP32 various dev board     : CS:  5, DC: 27, RST: 33, BL: 22, SCK: 18, MOSI: 23, MISO: nil
 * ESP32-C3 various dev board  : CS:  7, DC:  2, RST:  1, BL:  3, SCK:  4, MOSI:  6, MISO: nil
 * ESP32-S2 various dev board  : CS: 34, DC: 38, RST: 33, BL: 21, SCK: 36, MOSI: 35, MISO: nil
 * ESP32-S3 various dev board  : CS: 40, DC: 41, RST: 42, BL: 48, SCK: 36, MOSI: 35, MISO: nil
 * ESP8266 various dev board   : CS: 15, DC:  4, RST:  2, BL:  5, SCK: 14, MOSI: 13, MISO: 12
 * Raspberry Pi Pico dev board : CS: 17, DC: 27, RST: 26, BL: 28, SCK: 18, MOSI: 19, MISO: 16
 * RTL8720 BW16 old patch core : CS: 18, DC: 17, RST:  2, BL: 23, SCK: 19, MOSI: 21, MISO: 20
 * RTL8720_BW16 Official core  : CS:  9, DC:  8, RST:  6, BL:  3, SCK: 10, MOSI: 12, MISO: 11
 * RTL8722 dev board           : CS: 18, DC: 17, RST: 22, BL: 23, SCK: 13, MOSI: 11, MISO: 12
 * RTL8722_mini dev board      : CS: 12, DC: 14, RST: 15, BL: 13, SCK: 11, MOSI:  9, MISO: 10
 * Seeeduino XIAO dev board    : CS:  3, DC:  2, RST:  1, BL:  0, SCK:  8, MOSI: 10, MISO:  9
 * Teensy 4.1 dev board        : CS: 39, DC: 41, RST: 40, BL: 22, SCK: 13, MOSI: 11, MISO: 12
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 10 /* CS */, 6 /* SCK */, 7 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);

/* more fonts at: https://github.com/moononournation/ArduinoFreeFontFile.git */
#include "FreeMono8pt7b.h"
#include "FreeSansBold10pt7b.h"
#include "FreeSerifBoldItalic12pt7b.h"

float gs_rad; //stores angle from where to start in radinats
float ge_rad; //stores angle where to stop in radinats

//example values for testing, use the values you wish to pass as argument while calling the function
byte cx=120; //x center
byte cy=120; //y center
byte radius=110; //radius
byte percent=80; //needle percent

void setup(void)
{
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  /*
  for(int i=0;i<5;i++){
    gfx->setCursor(60, 225);
    uint8_t textSize = 10;
    gfx->setFont(&FreeSansBold24pt7b);
    gfx->setTextColor(WHITE);
    
    gfx->println(i);
    for (int c=0; c <= 100; c++) {  //this loop is for testing needle movement
        desenhaMedidor(cx,cy,radius,percent,c,0,100,i);
        delay(10);
    }
    delay(2000);
    gfx->fillScreen(BLACK);
    
  }
  */

  
  /*
  gfx->setCursor(60, 225);
  uint8_t textSize = 10;
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(WHITE);
  gfx->println("Km/h");

  delay(5000); // 5 seconds
  */
}

void velocimetro(int velocidade){
  for(int x=0;x<velocidade;x++){
    desenhaMedidor(cx,cy,100,95,x,0,130,3);
  }
}

void loop(){
  desenhaBussola(126);
  velocimetro(48);
  delay(5000);
  /*
  for(int i=0;i<360;i++){
    desenhaBussola(i);
    delay(25);
    gfx->fillScreen(BLACK);
  }
  /*
  gfx->setCursor(random(gfx->width()), random(gfx->height()));
  gfx->setTextColor(random(0xffff));
  uint8_t textSize = random(3);
  switch (textSize)
  {
  case 1:
    gfx->setFont(&FreeMono8pt7b);
    break;
  case 2:
    gfx->setFont(&FreeSansBold10pt7b);
    break;
  default:
    gfx->setFont(&FreeSerifBoldItalic12pt7b);
    break;
  }

  gfx->println("Hello World!");

  delay(1000); // 1 second
  */
}

void desenhaBussola(int heading){
   int minVal = 0;
   int maxVal = 180;
   int p = 80;
   int r=120;
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
  gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
  gfx->setTextSize(2);
  gfx->setTextColor(RED);
  gfx->println("N");
  i=((heading+90-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  xp = cx+(sin(i) * n1);
  yp = cy-(cos(i) * n1);
  xq = cx+(sin(i) * n2);
  yq = cy-(cos(i) * n2);
  gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
  gfx->setTextColor(WHITE);
  gfx->println("L");
  i=((heading+180-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  xp = cx+(sin(i) * n1);
  yp = cy-(cos(i) * n1);
  xq = cx+(sin(i) * n2);
  yq = cy-(cos(i) * n2);
  gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
  gfx->setTextColor(WHITE);
  gfx->println("S");
  i=((heading+270-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
  xp = cx+(sin(i) * n1);
  yp = cy-(cos(i) * n1);
  xq = cx+(sin(i) * n2);
  yq = cy-(cos(i) * n2);
  gfx->setCursor(((xp+xq)/2)-6,(((yp+yq)/2)-6));
  gfx->setTextColor(WHITE);
  gfx->println("O");
  //linhas
  n1=(r/100.00)*90; // calculate needle percent lenght
  n2=(r/100.00)*105; // calculate needle percent lenght
  for(int j=heading+15;j<heading+90;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(j-heading==45){
      gfx->drawLine(xp,yp,xq,yq, WHITE);
    } else {
      gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
    }
  }
  for(int j=heading+105;j<heading+180;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(j-heading==135){
      gfx->drawLine(xp,yp,xq,yq, WHITE);
    } else {
      gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
    }
  }
  for(int j=heading+195;j<heading+270;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(j-heading==225){
      gfx->drawLine(xp,yp,xq,yq, WHITE);
    } else {
      gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
    }
  }
  for(int j=heading+285;j<heading+360;j=j+15){
    i=((j-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
    xp = cx+(sin(i) * n1);
    yp = cy-(cos(i) * n1);
    xq = cx+(sin(i) * n2);
    yq = cy-(cos(i) * n2);
    if(j-heading==315){
      gfx->drawLine(xp,yp,xq,yq, WHITE);
    } else {
      gfx->drawLine(xp,yp,xq,yq, gfx->color565(128, 128, 128));
    }
  }
}

void desenhaMedidor(int x, byte y, byte r, byte p, int v, int minVal, int maxVal,  byte t ) {
  int n1=(r/100.00)*p; // calculate needle percent lenght
  int n2=(r/100.00)*110; // calculate needle percent lenght
  switch (t){
    case 0: { //left quarter
        float gs_rad=-1.572; //-90 degrees in radiant
        float ge_rad=0;
        float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
        int xp = x+(sin(i) * n1);
        int yp = y-(cos(i) * n1);
            gfx->drawLine(x,y,xp,yp, WHITE); 
        }
        break;
    case 1: { //right quarter, needle anticlockwise
  
        float gs_rad=0;
        float ge_rad=1.572; //90 degrees in radiant
        float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
        int xp = x+(cos(i) * n1);
        int yp = y-(sin(i) * n1);
            gfx->drawLine(x,y,xp,yp, WHITE); 
        }
        break; 
    case 2: { //upper half
        float gs_rad=-1.572;
        float ge_rad=1.572;
        float i=((v-minVal)*(ge_rad-gs_rad)/(maxVal-minVal)+gs_rad);
        int xp = x+(sin(i) * n1);
        int yp = y-(cos(i) * n1);
            gfx->drawLine(x,y,xp,yp, WHITE); 
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
            gfx->drawLine(xp,yp,xq,yq, WHITE); 
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
