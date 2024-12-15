bool ledState = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(D4, OUTPUT);
}

void loop() {
  Serial.println(random(100));
  ledState = !ledState;
  digitalWrite(D4, ledState);
  delay(3000);
}
