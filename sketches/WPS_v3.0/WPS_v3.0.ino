#include <SoftwareSerial.h>


// 10 goes to TX, is arduino RX
// 11 goes to RX, is arduino TX
SoftwareSerial sim(10, 11);
int GSM_RESET_PIN_HIGH = 8;
int GSM_RESET_PIN_LOW = 9;

int INIT_WAIT_MS = 1000;

int LISTEN_WAIT_TIMEOUT_MS = 5000;
int CHAR_READ_TIMEOUT_MS = 1000;


int PS_ANALOG_PIN = A0;
int MAX_ANALOG_VAL = 1023;


void(* resetFunc) (void) = 0; //declare reset function @ address 0


void setup() {
  Serial.begin(115200);
  sim.begin(9600);
  Serial.println();

  Serial.println(F("Begin setup"));

  setupGsmResetPins();
  testSensor();

  // Try gprs setup 3 times, if 3x failure, start blinking
  int setupGprsTries = 1;
  while (setupGprsTries <= 3) {
    if (setupGprs()) {
      break;
    }
    resetGsm();

    setupGprsTries++;
    if (setupGprsTries == 3) {
      Serial.println(F("3x GPRS setup failure, fatal"));
      blink(0);
    }
  }

  Serial.println(F("Setup complete"));
}

bool setupGprs() {
  Serial.print(F("Setting up GPRS in "));
  Serial.print(INIT_WAIT_MS);
  Serial.println(F("ms"));

  while (!isSimConnected()) {
    Serial.println(F("Waiting for connection"));
  }
  Serial.println(F("Connection established"));

  printToSim(F("AT+CMEE=2"), INIT_WAIT_MS);

  if (isGprsConnectionOk()) {
    Serial.println(F("SIM already connected"));
    return true;
  } else {
    char simOut[128];

    if (printToSim(F("AT+SAPBR=3,1,Contype,GPRS"), simOut, 128) != 0) {
      Serial.print(simOut);
    } else {
      return false;
    }
    if (printToSim(F("AT+SAPBR=3,1,APN,TM"), simOut, 128) != 0) {
      Serial.print(simOut);
    } else {
      return false;
    }
    if (printToSim(F("AT+SAPBR=1,1"), simOut, 128) != 0) {
      Serial.print(simOut);
    } else {
      return false;
    }
    if (printToSim(F("AT+SAPBR=2,1"), simOut, 128) != 0) {
      Serial.print(simOut);
    } else {
      return false;
    }
  }
  return true;
}

void setupGsmResetPins() {
  Serial.println(F("Setup GSM reset pins"));
  pinMode(GSM_RESET_PIN_HIGH, OUTPUT);
  pinMode(GSM_RESET_PIN_LOW, OUTPUT);
  digitalWrite(GSM_RESET_PIN_HIGH, HIGH);
  digitalWrite(GSM_RESET_PIN_LOW, LOW);
}

void testSensor() {
  Serial.println(F("Checking sensor health"));
  if (readSensor() != 0) {
    Serial.println(F("Sensor health ok"));
  } else {
    Serial.println(F("Sensor health fatal"));
    blink(1);
  }
}

// --- LOOP ---

void loop () {
  unsigned long loopStartMs = millis();
  checkGprsOrTryFix();

  float avgAnalogVal = readAvgAnalogVal();
  printReadingsToSerial(avgAnalogVal);
  float height = calcHeight(avgAnalogVal);
  sendData(height);

  delay(600000 - (millis() - loopStartMs));
}

bool sendData(float height) {
  // parse height to char[] and determine length
  char heightBuf[16];
  dtostrf(height, -1, 2, heightBuf);
  int heightStrLen;
  for (int i = 0; i < 16; i++) {
    if (((int) heightBuf[i]) == 0) {
      heightStrLen = i;
      break;
    }
  }
  
  // len of body syntax + heightStrLen + null byte
  int bodyLen = 11 + heightStrLen + 1;
  char body[bodyLen] = "{\"height\":";
  for (int i = 0; i < heightStrLen; i++) {
    body[i + 10] = heightBuf[i];
  }

  // Finish up body str
  body[bodyLen - 2] = '}';
  body[bodyLen - 1] = '\0';

  char simOut[128];

  printToSim(F("AT+CSQ"), simOut, 128);
  Serial.println(F("Signal quality:"));
  Serial.print(simOut);



  printToSim(F("AT+HTTPINIT"), simOut, 128);
  expectResultOrTryFix(simOut, "OK");
  printToSim(F("AT+HTTPPARA=CID,1"), simOut, 128);
  expectResultOrTryFix(simOut, "OK");
  printToSim(F("AT+HTTPPARA=URL,http://wasserstand-eisack.it/post"), simOut, 128);
  expectResultOrTryFix(simOut, "OK");
  printToSim(F("AT+HTTPPARA=CONTENT,application/json"), simOut, 128);
  expectResultOrTryFix(simOut, "OK");
  
  char httpDataStr[20] = "AT+HTTPDATA=";
  char bodyLenStr[3];
  itoa(bodyLen, bodyLenStr, 10);
  char httpDataStrEnd[6] = ",2000";
  httpDataStr[12] = bodyLenStr[0];
  httpDataStr[13] = bodyLenStr[1];
  for (int i = 0; i < 6; i++) {
    httpDataStr[i + 14] = httpDataStrEnd[i];
  }

  printToSim(httpDataStr, 500);
  printToSim(body, 500);

  printToSim(F("AT+HTTPACTION=1"), simOut, 128, 2000);
  Serial.print("httpaction:");
  Serial.print(simOut);
  printToSim(F("AT+HTTPTERM"), simOut, 128, 10000);
  expectResultOrTryFix(simOut, "OK");
}

void printReadingsToSerial(float avgAnalogVal) {
  float quotientVal = (float) avgAnalogVal / MAX_ANALOG_VAL;
  float voltVal = quotientVal * 5.0;
  float avgHeight = calcHeight(avgAnalogVal);

  Serial.print(F("Analog Value: "));
  Serial.println(avgAnalogVal);

  Serial.print(F("Quotient Value: "));
  Serial.println(quotientVal, 3);

  Serial.print(F("Voltage Value: "));
  Serial.print(voltVal, 3);
  Serial.println("V");

  Serial.print(F("Average height: "));
  Serial.print(avgHeight, 3);
  Serial.println(F("m"));

  Serial.println();
}

float calcHeight(float analogVal) {
    // Messung mit Neoprenkabel
    float cmNeopren = 2.479 * analogVal - 473.2;
    float mNeopren = cmNeopren / 100;

    // Plus oan Meter vierzig weil isch so
    float mNeoprenAdjusted = mNeopren + 1.375;
    return mNeoprenAdjusted;
}

float readAvgAnalogVal() {
  int sum = 0;
  int sumNum = 20;

  for (int i = 0; i < sumNum; i++) {
    int analogVal = readSensor();
    sum += analogVal;
  }

  float avgAnalogVal = (float) sum / sumNum;

  return avgAnalogVal;
}

int readSensor() {
  int analogVal = analogRead(PS_ANALOG_PIN);

  if (isnan(analogVal)) {
    Serial.println(F("Failed to read from Pressure sensor"));
    return 0;
  }

  return analogVal;
}

bool contains(char *s, char *target) {
  return strstr(s, target) != NULL;
}

int printToSim(char *s, char *buf, int bufSize, int delayMs) {
  delay(delayMs);

  sim.flush();
  sim.println(s);

  // wait for first char
  int startListen = millis();
  while (sim.available() == 0) {
    if (millis() - startListen > LISTEN_WAIT_TIMEOUT_MS) {
      return;
    }
  }

  int i = 0;
  unsigned long lastRead = millis();   // last time a char was available
  while (millis() - lastRead < CHAR_READ_TIMEOUT_MS){   
    while (sim.available()){
      char c = (char) sim.read();
      if (i < (bufSize - 1)) {
        buf[i++] = c;
      } else {
        return -1;
      }
      lastRead = millis();   // update the lastRead timestamp
    }
  }

  buf[i] = '\0';
  return i;
}

int printToSim(char *s, char *buf, int bufSize) {
  return printToSim(s, buf, bufSize, 100);
}

void printToSim(char *s, int delayMs) {
  delay(delayMs);
  sim.println(s);
}

void printToSim(char *s) {
  printToSim(s, 500);
}

int printToSim(const __FlashStringHelper* s, char *buf, int bufSize, int delayMs) {
  delay(delayMs);

  sim.flush();
  sim.println(s);

  // wait for first char
  int startListen = millis();
  while (sim.available() == 0) {
    if (millis() - startListen > LISTEN_WAIT_TIMEOUT_MS) {
      return;
    }
  }

  int i = 0;
  unsigned long lastRead = millis();   // last time a char was available
  while (millis() - lastRead < CHAR_READ_TIMEOUT_MS){   
    while (sim.available()){
      char c = (char) sim.read();
      if (i < (bufSize - 1)) {
        buf[i++] = c;
      } else {
        return -1;
      }
      lastRead = millis();   // update the lastRead timestamp
    }
  }

  buf[i] = '\0';
  return i;
}

int printToSim(const __FlashStringHelper* s, char *buf, int bufSize) {
  return printToSim(s, buf, bufSize, 100);
}

void printToSim(const __FlashStringHelper* s, int delayMs) {
  delay(delayMs);
  sim.println(s);
}

void printToSim(const __FlashStringHelper* s) {
  printToSim(s, 500);
}

void expectResultOrTryFix(char *s, char *target) {
  if (!contains(s, target)) {
    Serial.print(F("Error: "));
    Serial.print(s);
    Serial.print(F("Did NOT contain "));
    Serial.println(target);
    Serial.println(F("Will restart"));
    resetGsm();
    resetFunc();
  } else {
    Serial.print(s);
  }
}

void checkGprsOrTryFix() {
  if (!isGprsConnectionOk()) {
    Serial.println(F("Error: GPRS connection NOT OK, will try to recover"));
    resetGsm();
    if (!setupGprs()) {
      resetGsm();
      resetFunc();
    }
  } else {
    Serial.println(F("GPRS connection test succesful"));
  }
}

bool isGprsConnectionOk() {
  char simOut[128];
  printToSim(F("AT+SAPBR=2,1"), simOut, 64);
  Serial.print("test");
  Serial.print(simOut);
  return ((!contains(simOut, "0.0.0.0")) && (contains(simOut, "+SAPBR:")));
}

void resetGsm() {
  digitalWrite(GSM_RESET_PIN_HIGH, LOW);
  digitalWrite(GSM_RESET_PIN_LOW, HIGH);
  delay(2000);
  digitalWrite(GSM_RESET_PIN_HIGH, HIGH);
  digitalWrite(GSM_RESET_PIN_LOW, LOW);
}

bool isSimConnected() {
  char simOut[64];
  printToSim(F("AT+CREG?"), simOut, 64, 500);
  return (contains(simOut, "0,5"));
}

void blink(int mode) {
  // mode:
  // 0: blink single == gsm failure
  // 1: blink double == sensor failure
  pinMode(LED_BUILTIN, OUTPUT);

  if (mode == 0) {
    while (true) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
    }
  } else if (mode == 1) {
    while (true) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(400);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
    }
  } else {
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(1000);
  }
}