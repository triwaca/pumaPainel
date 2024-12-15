/*******************************************************************************
 * Velocimetro da PUMA
 * 
 * USAR ESP32C3 DEV
 * 
 * MAC 68:67:25:A0:8A:10
 * 
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/Org_01.h>

#include <esp_now.h>
#include <WiFi.h>

#include "iconePuma.c"
#define IMG_WIDTH 240
#define IMG_HEIGHT 78

int veloc = 0;
int ultimaVeloc = 0;
int combustivel = 0;
bool lockGps = false;

bool booting = true;

long timerCont = 0;
int cont = 0;
int ciclo = 2;

long timerSinal = 0;
long timerCurto = 0;

bool lastScreenHasAlert = false;

bool sinaleiraAtiva = true;

String horaParaExibir = "";
int horaAtual = 0;
int minutoAtual = 0;
String dataParaExibir = "";

bool luzBaixaAtiva = false;
bool luzAltaAtiva = true;
int dadosAtualizados = 0;

int temperatura = 0;
int tempMotor = 0;

// Mensagem de telemetria
typedef struct puma_telemetry {
    int velValue;
    int tempValue;
    int tempMotorValue;
    bool sinalValue;
    bool luzBaixaValue;
    bool luzAltaValue;
    int minutoValue;
    int horaValue;
    int combValue;
    bool gpsPosition;
} puma_telemetry;

puma_telemetry pumaData;

#include <Arduino_GFX_Library.h>

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 10 /* CS */, 6 /* SCK */, 7 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);

void setup(){
  // Init Display
  if (!gfx->begin())
  {
    //Serial.println("gfx->begin() failed!");
  }
  gfx->setRotation(1);
  gfx->fillScreen(BLACK);
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    debugMessage("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->draw16bitRGBBitmap(0, 81, (const uint16_t *)iconePuma, IMG_WIDTH, IMG_HEIGHT);
  delay(1000);
  
  //gfx->fillScreen(BLACK);
  gfx->setCursor(40, 190);
  gfx->setFont(&Org_01);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->println(WiFi.macAddress());
  delay(1000);
  /*
  gfx->draw16bitRGBBitmap(0, 0, (const uint16_t *)fundo, 240, 240);
  for(int i=0;i<220;i++){
    imprimeKmh(i);
    imprimeRpm(i/3);
    delay(10);
  }
  */
  booting = false;
  resetScreen();
  //exibeHora("00:00");
  
  //gfx->draw16bitRGBBitmap(171, 173, (const uint16_t *)baixa, 38, 38);
  //veloc = 63;
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&pumaData, incomingData, sizeof(pumaData));
  veloc = pumaData.velValue;
  luzBaixaAtiva = pumaData.luzBaixaValue;
  luzAltaAtiva = pumaData.luzAltaValue;
  sinaleiraAtiva = pumaData.sinalValue;
  horaAtual = pumaData.horaValue;
  minutoAtual = pumaData.minutoValue;
  temperatura = pumaData.tempValue;
  tempMotor = pumaData.tempMotorValue;
  combustivel = pumaData.combValue;
  if(combustivel<0) combustivel = 0;
  if(combustivel>99) combustivel = 99;
  lockGps = pumaData.gpsPosition; //diz se a recepção de GPS está OK.
  /*
  if(!booting){
    memcpy(&pumaData, incomingData, sizeof(pumaData));
    veloc = pumaData.velValue;
    if(pumaData.sinalValue) sinaleiraAtiva = true;
    horaParaExibir = pumaData.horaValue;
    dataParaExibir = pumaData.diaValue;
    luzBaixaAtiva = pumaData.luzBaixaValue;
    luzAltaAtiva = pumaData.luzAltaValue;
  } else {
    veloc = 199;
  }
  */
  dadosAtualizados = 0; //zera o contador de dados atualizados, indicando que está atualizado
}

void debugMessage(String text){
  gfx->setCursor(30, 190);
  gfx->setFont(&Org_01);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(2);
  gfx->println(text + "                      ");
}

void exibeHora(){
  if((horaAtual==0)and(minutoAtual==0)){
    //sem hora
  } else {
    horaParaExibir = (String)horaAtual + ":";
    if(minutoAtual<10) horaParaExibir = horaParaExibir + "0";
    horaParaExibir = horaParaExibir + (String)minutoAtual;
    gfx->fillRect(0, 6, 240, 28, BLACK);
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
    gfx->setCursor((posicHora/2)-12, 24);
    gfx->print(horaParaExibir);
  }
  /*
  gfx->fillRect(0, 6, 240, 28, YELLOW);
  gfx->setFont(&Org_01);
  gfx->setTextSize(4);
  gfx->setTextColor(WHITE, BLACK);
  if(horaAtual.length()>5){
    gfx->setCursor(73, 24);
    gfx->print("ERRO");
    gfx->setTextSize(2);
    gfx->setCursor(0, 115);
    gfx->setTextColor(WHITE, RED);
    gfx->print(horaAtual);
  } else {
    int desvioHora = 70;
    for(int i=0;i<horaAtual.length();i++){
      if(horaAtual.substring(i,i+1)=="1") desvioHora = desvioHora + 7;
    }
    gfx->setCursor(desvioHora, 24);
    gfx->print(horaAtual);
  }
  */
}

/*
void exibeData(String dataAtual){
  gfx->fillRect(0, 36, 240, 22, BLACK);
  gfx->setFont(&Org_01);
  gfx->setTextSize(3);
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(3);
  if(dataAtual.length()>10){
    gfx->setCursor(31, 48);
    gfx->print("GPS noDate");
    gfx->setTextSize(2);
    gfx->setCursor(0, 125);
    gfx->setTextColor(WHITE, RED);
    gfx->print(dataAtual);
  } else {
    int desvioData = 31;
    for(int i=0;i<dataAtual.length();i++){
      if(dataAtual.substring(i,i+1)=="1") desvioData = desvioData + 6;
    }
    gfx->setCursor(desvioData, 48);
    gfx->print(dataAtual);
  }
}
*/

void imprimeLuzes(){
  if(luzBaixaAtiva){
      gfx->draw16bitRGBBitmap(171, 173, (const uint16_t *)baixa, 38, 38);
    } else {
      if(luzAltaAtiva){
        gfx->draw16bitRGBBitmap(171, 173, (const uint16_t *)alta, 38, 38);
      } else {
        gfx->fillRect(171, 173, 38, 38, BLACK);
      } 
    }
}

void imprimeTemp(){
  gfx->fillRect(40, 32, 150, 28, BLACK);
  if((temperatura>0)and(temperatura<99)){
    int posicTemp = 140;
    int posicSimb = 0;
    int dezena = temperatura/10;
    int unidade = temperatura - (dezena*10);
    if(dezena==1){
      posicTemp = posicTemp - 6;
      posicSimb = posicSimb + 6;
    } else {
      if(dezena>0){
        posicTemp = posicTemp - 18;
        posicSimb = posicSimb + 18;
      }
    }
    if(unidade==1){
      posicTemp = posicTemp - 6;
      posicSimb = posicSimb + 6;
    } else {
      posicTemp = posicTemp - 18;
      posicSimb = posicSimb + 18;
    }
    gfx->setFont(&Org_01);
    gfx->setTextSize(3);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setCursor(posicTemp/2, 52);
    gfx->print(temperatura);
    gfx->print("    ");
    gfx->setTextSize(2);
    gfx->setCursor(posicTemp/2 + posicSimb, 44);
    gfx->print("o");
  }
  if(tempMotor>temperatura){ 
    int posicTemp = 340;
    int posicSimb = 0;
    int dezena = tempMotor/10;
    int unidade = tempMotor - (dezena*10);
    if(dezena==1){
      posicTemp = posicTemp - 6;
      posicSimb = posicSimb + 6;
    } else {
      if(dezena>0){
        posicTemp = posicTemp - 18;
        posicSimb = posicSimb + 18;
      }
    }
    if(unidade==1){
      posicTemp = posicTemp - 6;
      posicSimb = posicSimb + 6;
    } else {
      posicTemp = posicTemp - 18;
      posicSimb = posicSimb + 18;
    }
    gfx->setFont(&Org_01);
    gfx->setTextSize(3);
    if(tempMotor>80){
      gfx->setTextColor(RED, BLACK);
    } else if(tempMotor>70){
      gfx->setTextColor(YELLOW, BLACK);
    } else {
      gfx->setTextColor(WHITE, BLACK);
    }
    gfx->setCursor(posicTemp/2, 52);
    gfx->print(tempMotor);
    gfx->print("    ");
    gfx->setTextSize(2);
    gfx->setCursor(posicTemp/2 + posicSimb, 44);
    gfx->print("°");
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
  if(((veloc>99)and(ultimaVeloc<=99))or((veloc<=99)and(ultimaVeloc>99))){
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

void exibeAlerta(){
  if((veloc>=52)and(veloc<62)){
    gfx->fillRect(0, 60, 240, 15, ORANGE);
    gfx->fillRect(0, 156, 240, 15, ORANGE);
    lastScreenHasAlert = true;
  } else {
    if((veloc>=62)and(veloc<=76)){
      gfx->fillRect(0, 60, 240, 15, RED);
      gfx->fillRect(0, 156, 240, 15, RED);
      lastScreenHasAlert = true;
    } else {
      if((veloc>=86)){
        gfx->fillRect(0, 60, 240, 15, RED);
        gfx->fillRect(0, 156, 240, 15, RED);
        gfx->setFont(&Org_01);
        gfx->setTextSize(3);
        gfx->setTextColor(WHITE, RED);
        //gfx->setCursor(82, 70);
        //gfx->print("PERIGO!");
        gfx->draw16bitRGBBitmap(88, 88, (const uint16_t *)alerta, (const byte *)alerta_mask, 64, 64);
        lastScreenHasAlert = true;
      } else {
        if(lastScreenHasAlert){
          lastScreenHasAlert = false;
          if(veloc<70){
            gfx->fillRect(0, 60, 240, 15, BLACK);
            gfx->fillRect(0, 156, 240, 15, BLACK);
          } else {
            //resetScreen();
            gfx->fillRect(0, 60, 240, 15, BLACK);
            gfx->fillRect(0, 156, 240, 15, BLACK);
          }
        }
      }
    }
  }
}

void resetScreen(){
  gfx->fillScreen(BLACK);
  gfx->draw16bitRGBBitmap(83, 175, (const uint16_t *)miniPuma, 74, 75);
}


void loop(){
  if(timerCurto < millis()){
    timerCurto = millis()+500;
    dadosAtualizados++; //incrementa o valor de dados atualizados
    if(sinaleiraAtiva){
      gfx->fillRect(0, 174, 80, 40, BLACK);
      gfx->draw16bitRGBBitmap(35, 182, (const uint16_t *)sinal, 38, 20);
    } else {
      gfx->fillRect(35, 182, 38, 20, BLACK);
      gfx->setFont(&Org_01);
      gfx->setTextSize(4);
      if(combustivel<3){
        gfx->setTextColor(RED, BLACK);
      } else {
        gfx->setTextColor(WHITE, BLACK);
      }
      int posicComb = 30;
      if(combustivel/10 == 1) posicComb = posicComb+17;
      if(combustivel - round(combustivel/10)*10 == 1) posicComb = posicComb+17;
      gfx->fillRect(0, 174, 80, 40, BLACK);
      gfx->setCursor(posicComb, 190);
      if(combustivel<10){
        gfx->print("0" + (String)combustivel);
      } else {
        gfx->print(combustivel);
      }
      gfx->setCursor(57, 210);
      gfx->setTextSize(3);
      gfx->print("L");
    }
    if(dadosAtualizados<10){ //se a informação de velocidade tiver sido atualizada nos últimos 5 segundos
      if(lockGps){
        imprimeKmh(veloc);
      } else {
        imprimeKmh(veloc);
        gfx->fillRect(0, 102, 240, 20, BLACK);
        gfx->setCursor(42, 115);
        gfx->setFont(&Org_01);
        gfx->setTextColor(YELLOW);
        gfx->setTextSize(2);
        gfx->println("NO GPS SIGNAL");
      }
    } else {
      gfx->fillRect(0, 75, 240, 90, BLACK);
      gfx->setCursor(78, 115);
      gfx->setFont(&Org_01);
      gfx->setTextColor(RED);
      gfx->setTextSize(2);
      gfx->println("NO DATA");
      for(int i=0;i<30;i++){
        gfx->drawLine(0, i*2, 240, i*2, BLACK);
      }
      for(int i=0;i<120;i++){
        gfx->drawLine(i*2, 0, i*2, 60, BLACK);
      }
    }
    exibeAlerta();
  }
  if(timerSinal < millis()){
    sinaleiraAtiva = false;
    timerSinal = millis()+3000;
    if(dadosAtualizados<10){
      imprimeLuzes();
    } else {
      gfx->fillRect(171, 173, 38, 38, BLACK);
    } 
    exibeHora();
    imprimeTemp();
  }
}
