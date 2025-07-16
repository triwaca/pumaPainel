 /*
 *  PUMA HELTEC MAIN COMPUTER
 *  Daniel Almeida Chagas - 2024
 *  
 *  Componentes
 *  - Placa Heltec WIFI V2.0
 *  - Bateria 18650
 *  - Sensor de pressão/temp BMP230 via I2C
 *  - Sensor GPS RxTx Neo 6M (portas 17 e 23)
 *  - Módulo opto-acoplador de 4 entradas
 *    - Sinaleira
 *    - Luz alta
 *    - Luz baixa
 *    - Painel (para alarme de luz ligada)
 *  - Display OLED interno I2C
 *  - Circuito voltímetro da bateria
 *  - Transmissão de dados via ESPNOW
 *  - [REMOVIDO] Transmissão de telemetria via LORA
 *  - Buzzer para alarmes
 *  - Acumulador de dados de viagem
 *    - Velocidade média
 *    - RPM médio
 *    - Localização
 *  - [REMOVIDO] Encoder rotatório e 2 botões de painel
 *  - [REMOVIDO] Sensor de presença
 *  - Chave seletora de alarme de velocidade
 *  
 *                               cabo 1
 *                  |-----------------------------|
 *                  GND 3V3 3V3 36  37  38  39  34  35  32  33  25  26  27  14  12  13  21
 *                  []  []  []  []  AA  AA  AA  XX  XX  DD  DD  LL  []  DD  DD  II  XX* BB
 *              (prg)                   ---------------------------------------------
 *              -------                 |                                           |
 *              |     |                 |                                           |
 *              |     |                 |                                           |
 *              |     |                 |                                           |
 *              |     |                 |                                           |
 *              -------                 |                                           |
 *              (rst)                   ---------------------------------------------
 *                  []  []  []  []  []  []  []  II  II  XX  **  XX  XX  XX  II  XX  **  XX
 *                  GND 5V  Vxt Vxt RX  TX  RST 0   22  19  23  18  5   15  2   4   17  16
 *                  
 *                  Legenda:
 *                  ** GPS: pinos 17 RX e 23 TX
 *                  LL LED: pino 25
 *                  XX Não podem usar (OLED e LORA): Pino 4 SDA, e pino 15 SCL
 *                  AA Analog Read: Pinos 13 (bateria), 36 ref, 37 bateria, 38 combustível, e 39 temperatura
 *                  DD pinos digitais de leitura do painel
 *                  BB Buzzer
 *                  *  pino 13 normalmente liga a bateria, mas não funciona usando o WiFi
 *                  II pinos de input botões
 *                  
 *                  
 *                  
 *  
 *  Funcionalidades
 *  - Coleta de dados do dashboard (luzes espia)
 *  - Coleta de localização, data, proa e velocidade via GPS
 *  - Coleta de temperatura e altitude via barômetro
 *  - Coleta de tensão da bateria via circuto divisor de tensão
 *  - Transmissão de dados via ESPNOW
 *  - Alarmes:
 *    - Alarme sonoro/texto de luzes ligadas
 *    - Alarme sonoro/texto de nível de bateria
 *  - Avisos
 *    - Aviso sonoro/texto de problema de alternador
 *    - Aviso sonoro/texto de raios UV
 *  - Indicadores
 *    - Data, Hora e temperatura no display pequeno
 *      * Quando ligado, mostrando constantemente, quando desligado quando detectar movimento
 *    - Velocidade média no trecho, Velocidade média no percurso,
 *  
 *  
 *  
 */
#include "Arduino.h"
//#include "heltec.h"
#include <ArduinoUniqueID.h>
#include <esp_now.h>
#include <WiFi.h>
#include <TinyGPSPlus.h>
#include <SoftWire.h>
#include <AsyncDelay.h>
#include "SSD1306.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SimpleKalmanFilter.h>
#include <TimeLib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// BME280
#define SEALEVELPRESSURE_HPA (1009) //Check Internet for MSL pressure at our place to calc your elevation mASL
#define TEMP_CORR (-2)              //Manual correction of temp sensor (mine reads 2 degrees C too high)
#define ELEVATION (10)             //Enter your elevation mASL to calculate MSL pressure (QNH) at your place

Adafruit_BME280 bme; // I2C

#define PIN_SDA 4
#define PIN_SCL 15

SoftWire sw(PIN_SDA, PIN_SCL);

SSD1306 display(0x3C, PIN_SDA, PIN_SCL);

bool modoDebug = false;
String controladora = "indefinido";

//char* ssid = "NET_2G15E89F";
//char* password = "E215E89F";
//char* ssid = "pumagts";
//char* password = "";
char* ssid = "skynet";
char* password = "82at1nc&";


const char* serverName = "https://corisco.org/pumatrack/telemetry.php";
bool internet = false;
bool internetTry = false;
long internetTimer = 30000;
String telemetryIndicator = "NET";

String compile_date = __DATE__ " " __TIME__;
bool motorLigado = false;

float SLpressure_hPa;

int temperatura = 0;
int humidade = 0;
long timer = 0;

// The TinyGPSPlus object
TinyGPSPlus gps;
static const int RXPin = 17, TXPin = 23;
static const uint32_t GPSBaud = 9600;
double latGps = 0;
double lonGps = 0;
float accGps = 0;
int altGps = 0;
int satGps = 0;
long badSatGpsTimer = 0;
int velGps = 0;
int courseGps = 360;
bool lockGps = false;
bool lockTime = false;
long timerUpdatedGps = 0;
const int fusoHorario = -3;   // Brasil

String inputString = "";
bool stringComplete = false;
String txtHora = "";
String txtDia = "";
int veloc = 0;
int velocAnterior = 0;
// Pinos
bool led = false;
bool ledVerm = false;

/*Pinos para novo uso:
 *  12 0  GNDS  22 2
 * [*][*][*][*][*][*]
 * Conector na placa do computador
 * 
 * devem agora:
 * - Receber a posição da chave de velocidade
 * - Piscar o led vermelho de alerta
 */
int pinBotaoProg = 0;
int pinLedVeloc = 12;
int pinSeletorEsq = 22;
int pinSeletorDir = 2;

int pinRelaySound = 36;
int pinRelayInternet = 26;

int ledPin = 25;
int lbaixa = 27;
int lalta = 14;
int painel = 32; //pino para acordar o ESP do sleep
int sinal = 33;
int buzzer = 21;


//dados motor
bool valuePainel = false;
bool valueLBaixa = false;
bool valueLAlta = false;
bool valueSinal = false;

float voltagem = 0;
int leitura1 = 0;
int litros = 0;
SimpleKalmanFilter filtroComb(2, 2, 0.01);
/* Para evitar a flutuação do combustível para cima quando o carro estiver em movimento se fará uma heurística para controle:
 *  - A boia tende a mandar um valor maior de combustivel por estar sempre acima do nível. Quando o nível oscila, vai para cima
 *  - A boia marca um valor próximo do correto quando não há ondas no tanque.
 *  - O tanque nunca é abastecido em movimento.
 *  - Uma variável de litrosMax será usada para marcar esse valor quando o carro parado.
 *  - Para minimizar as ondas, o carro é considerado parado se estiver sem velocidade por 120 segundos. Nesse caso o valor de
 *  litrosMax é atualizado.
 *  - O valor do mostrador é exibido da função que mede a bomba, mas se o valor for maior que o litrosMax, é exibido o litrosMax
 */
long timerParado = 10000;
int litrosMax = 40;

int leitura2 = 0;
int tempMotor = 0;
int bateria = 0;
float XS = 0.0025;
uint16_t MUL = 1000;
uint16_t MMUL = 100;
SimpleKalmanFilter filtroTemp(2, 2, 0.01);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

String message_1 = "";
String message_2 = "";
String message_3 = "";
String message_4 = "";
String message_5 = "";
String message_6 = "";

long timerTelemetry = 0;

int telaAtual = 0;
int limiteVeloc = 50;
/*
 * 0 - debug
 * 1 - combustivel e voltagem
 * 2 - ECAM message
 * 3 - Hora
 */

 /*
  * --------------------------------
  * Còdigos para deep sleep
  * --------------------------------
  * 
  *  Ao ser desligado na chave (valuePanel false) o computador entra em modo sleep, com timer de alguns
  *  segundos (timerAcordado) para ligar e dar um ping. Como 
  */

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1800        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

long timerAcordado = 0;
bool pingSleep = false; //registra se conseguiu enviar a telemetria

// fim de códigos do deep sleep

// Códigos de computador de viagem
bool tempoRecebido = false;
bool iniciouJornada = false;
time_t inicioPercurso;
int metrosPercorridos = 0;
double lastLat = 0;
double lastLon = 0;
long millisParado = 0;
long ultimoMillisParado = 0;
int modoDirecao = 0; //1:lento 2:urbano  3:estrada
int velocMedia = 0;
SimpleKalmanFilter filtroVeloc(2, 2, 0.01);
int broadcastDataCouter = 0;


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

//dados para json
DynamicJsonDocument  doc(400);

esp_now_peer_info_t peerInfo;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  /*
  Serial.print("Last Telemetry Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.print("Delivery fail ");
  }
  */
}

String carUniqueID(){
  String id = "";
  for (size_t i = 0; i < UniqueIDsize; i++){
    if (UniqueID[i] < 0x10) id = id + "0";
    id = id + UniqueID[i];
  }
  return id;
}

const unsigned char gps_icon10 [] = {
  0x10, 0x01, 0xa8, 0x03, 0xd0, 0x01, 0xe0, 0x00, 0x40, 0x01, 0x95, 0x02, 0x05, 0x01, 0x19, 0x00, 
  0x02, 0x00, 0x1c, 0x00 
};

const unsigned char iot_icon10 [] = {
  0x00, 0x00, 0xfe, 0x01, 0xff, 0x03, 0x3d, 0x02, 0x6d, 0x03, 0x55, 0x03, 0x6d, 0x03, 0xff, 0x03, 
  0xfe, 0x01, 0x00, 0x00
};

const unsigned char net_icon10 [] = {
  0x00, 0x00, 0xfe, 0x01, 0x37, 0x03, 0xd3, 0x02, 0xd5, 0x03, 0x51, 0x02, 0xd7, 0x02, 0x37, 0x03, 
  0xfe, 0x01, 0x00, 0x00
};

const unsigned char hum_icon10 [] = {
  0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x07, 0x00, 0x07, 0x00, 0x0f, 0x80, 0x0f, 0x80, 0x0f, 0x80, 
  0x07, 0x00, 0x00, 0x00
};

void setup() {
  bipe();
  controladora = carUniqueID();
  pingSleep = false;
  pinMode(pinBotaoProg, INPUT);
  pinMode(pinLedVeloc, OUTPUT);
  pinMode(pinSeletorEsq, INPUT_PULLUP);
  pinMode(pinSeletorDir, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(pinRelaySound, OUTPUT);
  pinMode(pinRelayInternet, OUTPUT);
  pinMode(sinal, INPUT_PULLUP);
  pinMode(lbaixa, INPUT_PULLUP);
  pinMode(lalta, INPUT_PULLUP);
  pinMode(painel, INPUT_PULLUP);
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);
  delay(50);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.flipScreenVertically();
  delay(50);
  Serial.begin(115200);
  delay(250);
  pumaMessage("Init PUMA Main Computer 2024.2",false);
  pumaMessage(compile_date,false);
  pumaMessage(controladora,false);
  wakeupReason();
  pumaMessage("Serial USB at 115200bps",false);
  pumaMessage("GPS 9600bps at " + (String)RXPin + ", " + (String)TXPin,false);
  Serial1.begin(9600, SERIAL_8N1, RXPin, TXPin);

  //ALERTA ADC2 pins cannot be used when Wi-Fi is used.
  
  bool bme_status;
  bme_status = bme.begin(0x76);  //address either 0x76 or 0x77
  if (!bme_status) {
      display.clear();
      pumaMessage("No valid BME280 found",false);
      pumaMessage("Please check wiring!",false);
  } else {
    pumaMessage("Temp sensor BME280 OK",false);
  }

  //my best way to get correct indoor temperatures
  
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X16,  // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1,  // humidity
                  Adafruit_BME280::FILTER_X16,
                  Adafruit_BME280::STANDBY_MS_0_5 );

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA); //Para ESPNOW funcionar junto com WiFi
  //WiFi.mode(WIFI_STA);

  //connectWifi();

  if (esp_now_init() != ESP_OK) {
    pumaMessage("Error initializing ESP-NOW",false);
    return;
  } else {
    pumaMessage("ESP-NOW init OK",false);
  }
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    pumaMessage("Failed to add peer",false);
    return;
  }
  
  pumaMessage("System OK",false);
}

void connectWifi(){
  internet = false;
  internetTry = true;
  pumaMessage("WiFi connecting...",true);
  delay(50);
  WiFi.begin(ssid, password);
  for(int i=0;i<10;i++){
    if(WiFi.status() == WL_CONNECTED) {
      telemetryIndicator = "NET";
      internet = true;
      internetTry = false;
      pumaMessage("WI-FI " + (String)ssid + " OK!",false);
      break;
    }
    delay(1000);
    telemetryIndicator = "...";
    //Serial.print(".");
  }
}

void loop() {
  //timer para manter a conectividade da internet via WIFI
  if(millis()>internetTimer){
    internetTimer = millis() + 30000;
    /*
    if(WiFi.status() == WL_CONNECTED){
      internet = true;
    } else {
      internet = false;
    }
    if(!internet){  //se estiver sem internet
      connectWifi();
    }
    */
    if(timerUpdatedGps + 30000 > millis()){
      lockGps = false;
    }
    //outras funções lentas
    analisadorVeloc();
  }

  //timer para envio da telemetria via internet
  /*
  if(internet){
    if(millis()>timerTelemetry){
      telemetryIndicator = "NET";
      if(!valuePainel){
        //Se o motor não estiver ligado, o tempo de envio entre mensagens é de 20 minutos. 
        //Caso esteja ligado, a frequência de envios aumenta com a velocidade (mínimo de 1 por minuto). 
        timerTelemetry = millis() + 1200000;
      } else {
        if(veloc > velocAnterior + velocAnterior * 0.25){
          velocAnterior = veloc;
          timerTelemetry = millis() + 2000; //prox em 2 segundos
        } else if(veloc < velocAnterior - velocAnterior * 0.25){
          velocAnterior = veloc;
          timerTelemetry = millis() + 2000; //prox em 2 segundos
        } else {
          timerTelemetry = millis() + 10000; //10 em 10 segundos
        }
        if(velocAnterior < 4) velocAnterior = 4;
      }
      //Serial.println("Sending telemetry...");
      //WiFiClient client;
      HTTPClient http;
      //http.begin(client, serverName);
      http.begin(serverName);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      String httpRequestData = "controladora=";
      httpRequestData += controladora;
      httpRequestData += "&datareg=";
      if(lockTime) httpRequestData += (String)year() + "-" + (String)month() + "-" + (String)day() + " " + (String)hour() + ":" + (String)minute() + ":" + (String)second();
      httpRequestData += "&lat=";
      if(lockGps) {
        String latTemp = String(gps.location.lat(),6);
        httpRequestData += latTemp;
      }
      httpRequestData += "&lon=";
      if(lockGps) {
        String lonTemp = String(gps.location.lng(),6);
        httpRequestData += lonTemp;
      }
      httpRequestData += "&alt=";
      if(lockGps) httpRequestData += altGps;
      httpRequestData += "&acuracia=";
      if(lockGps) httpRequestData += accGps;
      httpRequestData += "&direcao=";
      if(lockGps) httpRequestData += courseGps;
      httpRequestData += "&comb=";
      httpRequestData += litros;
      httpRequestData += "&temp=";
      httpRequestData += temperatura;
      httpRequestData += "&tempmotor=";
      httpRequestData += tempMotor;
      httpRequestData += "&volt=";
      httpRequestData += voltagem;
      httpRequestData += "&humid=";
      httpRequestData += humidade;
      httpRequestData += "&humid=";
      httpRequestData += bateria;
      httpRequestData += "&veloc=";
      httpRequestData += velGps;
      httpRequestData += "&lbaixa=";
      httpRequestData += valueLBaixa;
      httpRequestData += "&lalta=";
      httpRequestData += valueLAlta;
      httpRequestData += "&painel=";
      httpRequestData += valuePainel;
      
      // Send HTTP POST request
      Serial.println(httpRequestData);
      int httpResponseCode = http.POST(httpRequestData);
      if(httpResponseCode==201) {
        telemetryIndicator = "TXOK";
        pumaMessage("telemetry SENT!",true);
        pingSleep = true;
        broadcastDataCouter++;
      } else {
        telemetryIndicator = "TXERR";
        pumaMessage("telemetry ERROR",false);
        pingSleep = false;
      }
      //Serial.println(httpResponseCode);
      http.end();
    }
  }
  */
  // TIMER de 1/4 segundo
  if(millis()>timer){
    /*
    if((WiFi.status() != WL_CONNECTED)&&internet){
      if(internet) pumaMessage("WI-FI disconnected!",true);
      internet = false;
      internetTry = false;
    }
    */
    led = !led;
    if(lockGps) digitalWrite(ledPin, led); //quando tem GPS, o led pisca
    if(veloc>limiteVeloc){
      ledVerm = !ledVerm;
      digitalWrite(pinLedVeloc, ledVerm);
    } else {
      ledVerm = false;
      digitalWrite(pinLedVeloc, ledVerm);
    }
    timer = millis() + 250;
    loadBmeData();
    readAdcData();
    display.clear();
    if(modoDebug){
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 0, "BME:");
      display.drawString(32, 0, (String)temperatura + "°C");
      display.drawString(82, 0, "Hum " + (String)humidade + "%");
      display.drawString(0, 10, "GPS:");
      if(lockTime){
        display.drawString(32, 10, txtHora);
        display.drawString(64, 10, txtDia);
      } else {
        display.drawString(32, 10, "no time!");
      }
      if(lockGps){
        display.drawString(6, 20, (String)latGps + " " + (String)lonGps);
        if(lockGps){
          display.drawString(70,20, "| " + (String)satGps + " Sats");
          display.drawString(6,30, "Acc." + (String)accGps + "m | Alt." + (String)altGps + "m");
        }
      }else {
        display.drawString(32, 20, "no GPS!");
      }
      if(courseGps!=360){
        display.drawString(6,40, (String)velGps + "Km/h | course " + (String)courseGps + "°");
      } else {
        display.drawString(6,40, (String)velGps + "Km/h");
      }
      display.drawString(0,50, "ADC:");
      display.drawString(32,50, (String)litros + "L ");
      display.drawString(58,50, (String)tempMotor + "°C ");
      display.drawString(82,50, "b" + (String)voltagem + "V");
      debugPainel();
    } else {
      if(!valuePainel){ //se desligado
        //telaAtual = 4;
      }
      if(telaAtual == 1){
        //modo hora
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 52, (String)temperatura + "°C");
        //display.drawXbm(32, 54, 10, 10, hum_icon10);
        //display.drawString(38, 52, (String)humidade + "%");
        //if(lockGps) display.drawString(80, 0, "GPS");
        if(lockGps) display.drawXbm(40, 54, 10, 10, gps_icon10);
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(127, 52, telemetryIndicator);
        if(internet) display.drawXbm(80, 54, 10, 10, net_icon10);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(Orbitron_Bold_36);
        display.drawString(64,0, txtHora);
        //display.drawString(64,0, "11:23");
        display.setFont(Orbitron_Medium_17);
        display.drawString(64,35, txtDia);
        //display.drawString(64,35, "05/04/2024");
      }
      if(telaAtual == 2){
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, message_1);
        display.drawString(0, 10, message_2);
        display.drawString(0, 20, message_3);
        display.drawString(0, 30, message_4);
        display.drawString(0, 40, message_5);
        display.drawString(0, 50, message_6);
      }
      if(telaAtual == 3){
        display.setFont(Orbitron_Medium_17);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.clear();
        display.drawString(64, 0, "Combustivel");
        display.drawRect(0,20,127,27);
        //int nivel = map(leitura1,1050,2555,123,2);
        int nivel = 0;
        if(leitura1<1680){
          nivel = map(leitura1,1050,1680,123,93);
        } else if(leitura1<2096){
          nivel = map(leitura1,1680,2096,93,63);
        }else if(leitura1<2318){
          nivel = map(leitura1,2096,2318,63,33);
        } else if(leitura1<2555){
          nivel = map(leitura1,2318,2555,33,2);
        } else {
          nivel = 2;
        }
        if(nivel<2) nivel = 2;
        if(nivel>123) nivel = 123;
        display.fillRect(2,22,nivel,23);
        
        display.drawString(64, 51, (String)litros + " L - " + (String)leitura1);
      }
      /*
      if(telaAtual == 4){
        //modo vigilancia
        display.setFont(Orbitron_Medium_17);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.clear();
        display.drawString(64, 5, "ATIVADO");
        display.drawString(64, 25, "VIGILANCIA!");
        display.drawString(64, 45, "T 60s");
        display.setColor(BLACK);
        for(int i=0;i<32;i++){
          display.drawLine(0,i*2,127,i*2);
        }
        
        for(int i=0;i<64;i++){
          display.drawLine(i*2,0,i*2,63);
        }
        
        display.setColor(WHITE);
        if(!internet) drawBatLevel();
        //display.drawString(64,0, txtHora);
        if((pingSleep)||(millis()>30000)){ //se tiver enviado a telemetria ou passado 60 segundos
          display.setFont(Orbitron_Medium_17);
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.clear();
          display.drawString(64, 5, "SLEEP");
          display.drawString(64, 25, "MODE");
          display.drawString(64, 45, "T " + String(TIME_TO_SLEEP) + " s");
          display.setColor(BLACK);
          for(int i=0;i<32;i++){
            display.drawLine(0,i*2,127,i*2);
          }
          display.display();
          esp_sleep_enable_ext0_wakeup(GPIO_NUM_32,0);
          esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
          delay(3000);
          sleepMode();
        }
      }
      */
    }
    //drawBatLevel();
    display.display();
    broadcastData();
  }
  //recebe dados do GPS
  while (Serial1.available() > 0){
    if(gps.encode(Serial1.read())){
      loadGpsData();
      timerUpdatedGps = millis();
    }
  }
  readButtonsData();
  readPanelData();
}

void debugPainel(){
  if(digitalRead(sinal)) display.drawLine(0,0,31,0);
  if(digitalRead(lbaixa)) display.drawLine(32,0,63,0);
  if(digitalRead(lalta)) display.drawLine(64,0,95,0);
  if(digitalRead(painel)) display.drawLine(96,0,128,0);
}

void pumaMessage(String txt, bool silent){
  message_1 = message_2;
  message_2 = message_3;
  message_3 = message_4;
  message_4 = message_5;
  message_5 = message_6;
  message_6 = txt;
  //Serial.println(txt);
  if(!silent){
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, message_1);
    display.drawString(0, 10, message_2);
    display.drawString(0, 20, message_3);
    display.drawString(0, 30, message_4);
    display.drawString(0, 40, message_5);
    display.drawString(0, 50, message_6);
    display.display();
    delay(200);
  }
}

void loadBmeData(){
   bme.takeForcedMeasurement();
   float temp = bme.readTemperature();
   temp = temp + TEMP_CORR;
   float humi = bme.readHumidity();
   float alti = bme.readAltitude(SEALEVELPRESSURE_HPA);
   
   temperatura = temp;
   humidade = humi;
}

void readPanelData(){
  valuePainel = !digitalRead(painel);
  if(valuePainel){
    valueSinal = digitalRead(sinal);
  } else {
    valueSinal = 0;
  }
  valueLBaixa = !digitalRead(lbaixa);
  if(!valueLBaixa){
    valueLAlta = !digitalRead(lalta);
  } else {
    valueLAlta = 0;
  }
}

void readButtonsData(){
  if(modoDebug){
    if(digitalRead(pinBotaoProg)) display.drawLine(0,63,31,63);
    //if(digitalRead(pinBotaoHome)) display.drawLine(32,63,63,63);
    if(digitalRead(pinSeletorEsq)) display.drawLine(64,63,95,63);
    if(digitalRead(pinSeletorDir)) display.drawLine(96,63,127,63);
  } else {
    if(!digitalRead(pinBotaoProg)) {
      modoDebug = false;
      bipe();
    }
    /*
    if(!digitalRead(pinBotaoHome)) {

      bipe();
    }
    */
    if(!digitalRead(pinSeletorEsq)) {
      telaAtual = 1;
      limiteVeloc = 50;
    } else if(!digitalRead(pinSeletorDir)) {
      telaAtual = 3;
      limiteVeloc = 80;
    } else {
      telaAtual = 2;
      limiteVeloc = 60;
    }
  }
}

void readAdcData(){
  /*
  uint16_t c1  =  analogRead(13); //*XS*MUL;
  delay(100);
  uint16_t c2  =  analogRead(36)*0.769 + 150;
  */
  voltagem = ((float)map(analogRead(37),2430,2970,120,150))/10;
  if(voltagem<2) voltagem = 0;
  // 5V - 930
  // 12V - 2430
  // 15V - 2970
  //voltagem = analogRead(37);
  leitura1 = analogRead(38);
  leitura2 = analogRead(39);
  litros = filtroComb.updateEstimate(calculoLitros(leitura1));
  tempMotor = filtroTemp.updateEstimate(map(leitura2,2781,1258,30.00,72.00));
  if(tempMotor < temperatura) tempMotor = temperatura;
  //bateria = map(c1,4900,3200,100,0);
  //bateria = c1;
  //if(bateria>100) bateria = 100;
  if(litros<0) litros = 0;
}

void loadGpsData(){
  satGps = gps.satellites.value();
  if((satGps<3)&&(millis()>badSatGpsTimer + 30000)) {
    badSatGpsTimer = millis();
    pumaMessage("GPS bad reception!",true);
  } 
  if(gps.course.isValid()){
    courseGps = gps.course.deg();
  } else {
    courseGps = 360;
  }
  if(satGps>3){
    lockGps = true;
    timerUpdatedGps = millis();
    velGps = gps.speed.kmph();
    if(velGps < 4){
      /* CARRO PARADO
       * Usa a variável ultimoMillisParado para marcar o tempo anterior parado, para no ciclo 
       * seguinte acrescentar à variável millisParado
       */
      millisParado += millisParado + (millis()-ultimoMillisParado);
      velGps = 0;
      courseGps = 360;
      ultimoMillisParado = millis();
    } else {
      // CARRO EM MOVIMENTO
      ultimoMillisParado = 0;
      velocMedia = filtroVeloc.updateEstimate(veloc);
    }
  } else {
    lockGps = false;
    velGps = 0;
    courseGps = 360;
  }
  if (gps.location.isValid()){
    latGps = gps.location.lat();
    lonGps = gps.location.lng();
    altGps = gps.altitude.meters();
    accGps = gps.hdop.hdop();
    veloc = (int)gps.speed.kmph();
  } else {
    lockGps = false;
  }
  if (gps.time.isValid()){
    if(gps.date.year() > 2000){
      if((gps.date.year()!=year())||(gps.time.minute()!=minute())) lockTime = false; //verifica se a hora está desatualizada
      if(!lockTime){
        lockTime = true;
        setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
        adjustTime(fusoHorario * 3600);
        pumaMessage("dateTime updated by GPS",true);
      }
      //inicia a construção da hora
      txtHora = "";
      if (hour() < 10) txtHora += "0";
      txtHora += (String)hour();
      txtHora += ":";
      if (minute() < 10) txtHora += "0";
      txtHora += minute();
      //construção string dia
      txtDia = "";
      if(day()<10) txtDia += "0";
      txtDia = day();
      txtDia += "/";
      if(month()<10) txtDia += "0";
      txtDia += month();
      txtDia += "/";
      txtDia += year();
      //pumaData.horaValue = hour();
      //pumaData.minutoValue = minute();
    }
  } else {
    lockTime = false;
    txtHora = "??:??";
    txtDia = "??/??/????";
    pumaMessage("GPS dateTime error!",false);
    //pumaData.horaValue = 0;
    //pumaData.minutoValue = 0;
  }
}

void drawBatLevel(){
  int posY = 55;
  display.drawRect(113,posY + 2,2,3);
  display.drawRect(114,posY,13,7);
  int posicao = map(analogRead(13),2100,1350,11,0);
  display.fillRect(125-posicao,posY + 2,posicao,3);
}

double readLipoVoltage(byte pin){
  double reading = analogRead(pin); // Recalibrado
  if(reading < 1 || reading >= 4095)
  return (-0.000000000009824 * pow(reading,3) + 0.000000016557283 * pow(reading,2) + 0.000854596860691 * reading + 0.065440348345433) * 2.3;
}

void bipe(){
  tone(buzzer, 523);
  delay(200);
  noTone(buzzer);
}

void somSinaleira(bool ligada, bool tom){
  if(ligada){
    if(tom){
      tone(buzzer, 262);
    } else {
      tone(buzzer, 294);
    }
    delay(50);
    noTone(buzzer);
  } else {
    noTone(buzzer);
  }
}

void broadcastData(){
  //preenche mensagem para broadcast
  pumaData.sinalValue = valueSinal;
  doc["sinal"] = valueSinal;
  pumaData.luzBaixaValue = valueLBaixa;
  doc["baixa"] = valueLBaixa;
  pumaData.luzAltaValue = valueLAlta;
  doc["alta"] = valueLAlta;
  txtHora = "";
  pumaData.horaValue = hour();
  doc["hora"] = hour();
  pumaData.minutoValue = minute();
  doc["min"] = minute();
  if (lockGps)  {
    pumaData.velValue = veloc;
    doc["vel"] = veloc;
  } else {
    pumaData.velValue = 0;
    doc["vel"] = 0;
  }
  pumaData.tempValue = temperatura;
  doc["temp"] = temperatura;
  pumaData.tempMotorValue = tempMotor;
  doc["tempMotor"] = tempMotor;
  doc["humi"] = humidade;
  pumaData.combValue = litros;
  doc["comb"] = litros;
  pumaData.gpsPosition = lockGps;
  doc["gps"] = lockGps;
  doc["bat"] = voltagem;
  doc["direc"] = courseGps;
  doc["limite"] = limiteVeloc;
  if(painel) esp_now_send(broadcastAddress, (uint8_t *) &pumaData, sizeof(pumaData));
  //dados via JSON na serial
  serializeJson(doc, Serial);
  Serial.println();
  
}

double calculoLitros(int x){
  /*
   * Novos dados com sensor longo 2024.2
   * 
   * reserva 4L
   * 2960 - Zero litros (medido com o sensor fora do tanque)
   * 2750 - 5L
   * 2450 - 10L
   * 2190 - 15L
   * 1810 - 20L
   * 
   */
  int resultado = 0;
  if(x<2190){
    resultado = map(x,1810,2190,20,15);
  } else if(x<2450){
    resultado = map(x,2190,2450,15,10);
  }else if(x<2750){
    resultado = map(x,2450,2750,10,5);
  } else if(x<2960){
    resultado = map(x,2750,2960,5,0);
  } else {
    resultado = 0;
  }
  if(resultado<0) resultado = 0;
  if(veloc==0){
    if(millis()>timerParado + 60000){
      litrosMax = resultado;
    }
  } else {
    timerParado = millis();
  }
  if(resultado>litrosMax) resultado = litrosMax;
  return resultado;
}

void wakeupReason(){
  ++bootCount;
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0 : pumaMessage("Wakeup ext sig RTC_IO",false); break;
    case ESP_SLEEP_WAKEUP_EXT1 : pumaMessage("Wakeup ext sig RTC_CNTL",false); break;
    case ESP_SLEEP_WAKEUP_TIMER : pumaMessage("Wakeup by timer",false); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : pumaMessage("Wakeup by touchpad",false); break;
    case ESP_SLEEP_WAKEUP_ULP : pumaMessage("Wakeup by ULP program",false); break;
    default : pumaMessage("Wakeup: " + wakeup_reason,false); break;
  }
  pumaMessage("Boot n: " + String(bootCount), false);
}

void sleepMode(){
  pumaMessage("Going to sleep...",false);
  Serial.flush();
  esp_deep_sleep_start();
}

void analisadorVeloc(){
  if(velocMedia>10){
    if(velocMedia<20){
      if(modoDirecao!=1){
        modoDirecao = 1;
        pumaMessage("Modo lento " + (String)velocMedia + "Km/h", false);
      }
    } else if(velocMedia<60){
      if(modoDirecao!=2){
        modoDirecao = 2;
        pumaMessage("Modo urbano " + (String)velocMedia + "Km/h", false);
      }
    } else {
      if(modoDirecao!=3){
        modoDirecao = 3;
        pumaMessage("Modo estrada " + (String)velocMedia + "Km/h", false);
      }
    }
  }
}

void tripComputer(){
  //calcula os dados da viagem caso o carro esteja ligado
  if(valuePainel){
    //se o relógio estiver atualizado
    if(lockTime){
      if(veloc>5){
        //se não iniciou a jornada, inicie! Se já iniciou, compute
        if(!iniciouJornada){
          iniciouJornada = true;
          inicioPercurso = now();
          pumaMessage("Trip begin " + constroiStringTempo(inicioPercurso),false);
        } else {
          if(lastLat != 0) metrosPercorridos += TinyGPSPlus::distanceBetween(lastLat,lastLon,gps.location.lat(),gps.location.lng());
          lastLat = gps.location.lat();
          lastLon = gps.location.lng();
        }
      }
    }
  } else {
    if(iniciouJornada){
      //fim da jornada
      pumaMessage("Trip end!", false);
      viewEndTripScreen();
    } else {
      //carro ainda não iniciou a jornada nem andou
    }
  }
}

void viewEndTripScreen(){
  //exibe os dados da viagem terminada
  display.setFont(Orbitron_Medium_17);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.clear();
  display.drawString(64, 0, "Trip: " + constroiTempoViagem(inicioPercurso));
  display.drawString(64, 21, (String)constroiStringTempo(inicioPercurso) + " to " + (String)constroiStringTempo(now()));
  display.drawString(64, 42, "AVG " + (String)velocMedia + " Km/h Data: " + (String)broadcastDataCouter);
  display.drawString(64, 63, (String)constroiDistanciaViagem());
  display.display();
  delay(5000);
}

String constroiStringTempo(time_t tempTime){
  String timeFinal = (String)hour(tempTime) + ":";
  if(minute(tempTime)<10) timeFinal += "0";
  timeFinal += (String)minute(tempTime);
  return timeFinal;
}

String constroiTempoViagem(time_t tempTime){
  int segundosPassados = second() - second(tempTime);
  int minutosPassados = minute() - minute(tempTime);
  int horasPassadas = hour() - hour(tempTime);
  int segundosTotais = segundosPassados + minutosPassados*60 + horasPassadas*3600;
  int minutosTemp = segundosTotais/60;
  String timePassado = "";
  if(minutosTemp>60){
    int horasTemp = minutosTemp/60;
    minutosTemp = minutosTemp - horasTemp*60;
    timePassado = (String)horasTemp + "h ";
  }
  if(minutosTemp<10){
    timePassado += "0" + (String)minutosTemp;
  } else {
    timePassado += (String)minutosTemp;
  }
  timePassado += "m ";
  int segundosTemp = segundosTotais - (minutosTemp * 60);
  if(segundosTemp<10){
    timePassado += "0" + (String)segundosTemp + "s";
  } else {
    timePassado += (String)segundosTemp + "s";
  }
  return timePassado;
}

String constroiDistanciaViagem(){
  String distTrip = "";
  float kmPercorridos = (float)metrosPercorridos/(float)1000;
  if(metrosPercorridos<500){
    distTrip += (String)metrosPercorridos + "m";
  } else {
    distTrip += (String)kmPercorridos + "Km";
  }
  return distTrip;
}
