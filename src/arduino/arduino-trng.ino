int count = 0;
int prev = 0;
const byte interruptPin = 21;
char bufferOutput[4];
unsigned int a,b,c,d;
int start = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);
  delay(2000);
  start = 1;
}

void loop() {
}

void blink() {
  count = (int)((micros()/8)%24576);
  if(abs(count-prev) < 64 || start == 0) {
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
