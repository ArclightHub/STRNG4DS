// See micros reference page for multiplier: https://www.arduino.cc/reference/en/language/functions/time/micros/
int boardTimingPrecision = 8;

// Debounce time period, you may need to make this smaller if your beta source is very strong, however it is not recommended.
// A larger number improves the quality of your random at the cost of lowering the bit stream rate.
int debounceTime = 386;

// Vars
int deboundeCeiling = 24576 - debounceTime;
int count = 0;
int prev = 0;
int start = 0;
int delta;
const byte interruptPin = 2; // Trigger pin, change as needed.
char bufferOutput[4];
unsigned int a,b,c,d;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  //pinMode(interruptPin, INPUT_PULLUP); // Some counter boards need pullup, uncomment if yours pulls to ground as trigger.
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);
  delay(500);
  start = 1;
}

void loop() {
}

void blink() {
  count = (int)((micros()/boardTimingPrecision)%24576);
  delta = abs(count-prev);
  if(
    delta < debounceTime
    || delta > deboundeCeiling
    || start == 0
  ) {
    //Debounce
    prev = count;
    return;
  }
  c,d = count%256;
  a = c>>4;
  b = (d&0x0f);
  sprintf(bufferOutput, "|%x|%x",a,b);
  if (Serial.availableForWrite()) {
    Serial.write(bufferOutput);
  }
  prev = count;
}
