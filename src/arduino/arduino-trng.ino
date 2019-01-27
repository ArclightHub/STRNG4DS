int count = 0;
bool state = false;
const byte interruptPin = 21;
bool mutex = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.write("Loading");
  delay(500);
  pinMode(interruptPin, INPUT_PULLUP);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE); 
}

void loop() {
  count++;
  if(count > 16384) count = 0;
}

void blink() {
  if(mutex == true){
    return;
  } else mutex = true;
  state = !state;
  if(state){
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  char buffer[1];
  char bufferOutput[2];
  itoa(count%16,buffer,16);
  sprintf(bufferOutput, "|%s",buffer);
  if (Serial.availableForWrite()) {
    Serial.write(bufferOutput);
  }
  mutex = false;
}
