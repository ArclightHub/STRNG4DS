unsigned int count = 0;
const byte interruptPin = 21;
char bufferOutput[4];
unsigned int a,b,c,d,prev;

void setup() {
  prev = 0;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.write("Loading");
  delay(500);
  pinMode(interruptPin, INPUT_PULLUP);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE); 
}

void loop() {
}

void blink() {
  count = (unsigned int)(micros()%65535);
  if(count-prev < 64) {
    //Debounce
    prev = count;
    return;
  }
  c = count%256;
  a = c>>4;
  d = count%256;
  b = (d&0x0f);
  sprintf(bufferOutput, "|%x|%x",a,b);
  if (Serial.availableForWrite()) {
    Serial.write(bufferOutput);
  }
  prev = count;
}
