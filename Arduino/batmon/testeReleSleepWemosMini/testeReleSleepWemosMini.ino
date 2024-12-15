
int com_port = D1;
int mid_port = D2;
int led_port = D4;

void setup() {
  pinMode(led_port, OUTPUT);
  pinMode(com_port, OUTPUT);
  pinMode(mid_port, OUTPUT); 
}

void loop() {
  digitalWrite(led_port, HIGH);
  digitalWrite(com_port, HIGH);
  digitalWrite(mid_port, LOW);
  //Serial.println("COM");
  delay(2000);
  digitalWrite(led_port, LOW);
  digitalWrite(com_port, LOW);
  digitalWrite(mid_port, HIGH);
  //Serial.println("MID");
  delay(2000);
  //Serial.println("Entrando em Deep-Sleep por 30 segundos");
  ESP.deepSleep(10e6); // 30e6 = 30000000 microssegundos = 30 segundos
}
