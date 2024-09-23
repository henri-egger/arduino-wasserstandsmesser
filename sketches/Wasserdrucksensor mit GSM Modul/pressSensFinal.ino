#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SoftwareSerial.h>

// 10 goes to TX, is arduino RX
// 11 goes to RX, is arduino TX
SoftwareSerial sim(10, 11);

int LCD_I2C_ADDRESS = 0x27;
int LCD_WIDTH = 20;
int LCD_HEIGHT = 4;
int LCD_POWER_PIN = 12;

hd44780_I2Cexp lcd;

int PS_ANALOG_PIN = A0;
int MAX_ANALOG_VAL = 1023;

float DELAY_MINUTES = 10;

void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial.println("Setting up display...");
  setupLcd();

  Serial.println("Setting up SIM...");
  setupSim();

  Serial.println("Setup complete!");
}

void setupLcd() {
  pinMode(LCD_POWER_PIN, OUTPUT);
  digitalWrite(LCD_POWER_PIN, HIGH);
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.clear();

  return;
}

void setupSim() {
  sim.begin(9600);
  delay(1000);

  // Set detailed error messages
  printToSim("AT+CMEE=2");

  // Setup GPRS connection
  printToSim("AT+SAPBR=3,1,Contype,GPRS");
  printToSim("AT+SAPBR=3,1,APN,TM");
  printToSim("AT+SAPBR=1,1");
  printToSim("AT+SAPBR=2,1");

  return;
}

void loop() {
  float avgAnalogVal = readAvgAnalogVal();

  printCalcValues(avgAnalogVal);

  float height = calcHeight(avgAnalogVal);

  sendData(height);
  delay(minutesToMillies(DELAY_MINUTES));
}

void sendData(float height) {
  String body = "{\"height\":" + String(height) + "}";

  printToSim("AT+HTTPINIT");
  printToSim("AT+HTTPPARA=CID,1");
  printToSim("AT+HTTPPARA=URL,http://wasserstand-eisack.it/post");
  printToSim("AT+HTTPPARA=CONTENT,application/json");
  
  sim.println("AT+HTTPDATA=" + String(body.length()) + ",2000");
  delay(1000);
  sim.println(body);
  delay(2000);

  printToSim("AT+HTTPACTION=1");
  delay(10000);
  printToSim("AT+HTTPTERM");
}

#define TIMEOUT 1000

void printToSim(String command){
  sim.println(command);
  while (sim.available() == 0);  // wait for first char

  unsigned long lastRead = millis();   // last time a char was available
  while (millis() - lastRead < TIMEOUT){   
    while (sim.available()){
      Serial.write(sim.read());
      lastRead = millis();   // update the lastRead timestamp
    }
  }
  // No need for extra line feed since most responses contain them anyways
}

float readAvgAnalogVal() {
  int sum = 0;
  int sumNum = 20;

  for (int i = 0; i < sumNum; i++) {
    float analogVal = readSensor();
    sum += analogVal;
  }

  float avgAnalogVal = (float) sum / sumNum;

  return avgAnalogVal;
}

float readSensor() {
  int analogVal = analogRead(PS_ANALOG_PIN);

  if (isnan(analogVal)) {
    Serial.println("Failed to read from Pressure sensor");
    return NULL;
  }

  return analogVal;
}

void printCalcValues(float avgAnalogVal) {
  float quotientVal = (float) avgAnalogVal / MAX_ANALOG_VAL;
  float voltVal = quotientVal * 5.0;
  float avgHeight = calcHeight(avgAnalogVal);
  
  printReadingsToSerial(avgAnalogVal, quotientVal, voltVal, avgHeight);
  printReadingsToLcd(avgAnalogVal, avgHeight);

  return;
}

// For avg values
float calcHeight(float analogVal) {
    // // Alte Messung
    // return 2.466 * analogVal - 245.1;

    // // Alle Werte, angepasst
    // return 2.499 * analogVal - 258.2;

    // Meterband Werte, angepasst
    // return 2.487 * analogVal - 252.8;

    // Messung mit Neoprenkabel
    float cmNeopren = 2.479 * analogVal - 473.2;
    float mNeopren = cmNeopren / 100;

    // Plus oan Meter vierzig weil isch so
    float mNeoprenAdjusted = mNeopren + 1.375;
    return mNeoprenAdjusted;
}

void printReadingsToSerial(int avgAnalogVal, float quotientVal, float voltVal, float avgHeight) {
  Serial.print("Analog Value: ");
  Serial.println(avgAnalogVal);

  Serial.print("Quotient Value: ");
  Serial.println(quotientVal, 3);

  Serial.print("Voltage Value: ");
  Serial.print(voltVal, 3);
  Serial.println("V");

  Serial.print("Average height: ");
  Serial.print(avgHeight, 3);
  Serial.println("m");

  Serial.println();

  return;
}

void printReadingsToLcd(int avgAnalogVal, float avgHeight) {
  lcd.clear();

  String aText = "Avg Analog: " + String(avgAnalogVal);
  lcd.setCursor(0, 0);
  lcd.print(aText);

  String vhText = "Avg Height: " + String(avgHeight, 2) + "m";
  lcd.setCursor(0, 1);
  lcd.print(vhText);

  return;
}

int minutesToMillies(float minutes) {
  return (int) (minutes * 60 * 1000);
}
