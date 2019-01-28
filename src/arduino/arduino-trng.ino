int count = 0;
int prev = 0;
int start = 0;
int delta;
const byte interruptPin = 21;
char bufferOutput[4];
unsigned int a,b,c,d;

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
  delta = abs(count-prev);
  if(
    delta < 64
    || delta > 24512
    || start == 0
  ) {
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
