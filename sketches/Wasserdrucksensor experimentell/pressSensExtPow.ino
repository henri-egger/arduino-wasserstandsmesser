#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

int LCD_I2C_ADDRESS = 0x27;
int LCD_WIDTH = 20;
int LCD_HEIGHT = 4;
int LCD_POWER_PIN = 12;

hd44780_I2Cexp lcd;

int PS_ANALOG_PIN = A0;
int MAX_ANALOG_VAL = 1023;

int autoReadPauseMillis = 100;

void setup() {
  Serial.begin(9600);
  Serial.println();

  Serial.println("Setting up display...");
  setupLcd();

  Serial.println("Setting up sensor...");
  setupPs();

  Serial.println("Setup complete!");
}

void setupPs() {
  // Pressure sensor is powered by pin 12
  // pinMode(PS_POWER_PIN, OUTPUT);
  // digitalWrite(PS_POWER_PIN, HIGH);
}

void setupLcd() {
  // Pressure sensor is powered by pin 12
  pinMode(LCD_POWER_PIN, OUTPUT);
  digitalWrite(LCD_POWER_PIN, HIGH);
  Serial.print("Pin set");
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  Serial.print("All set");
  lcd.clear();
  
}

bool autoRead = true;

void loop() {
  if (autoRead) {
    readSensorAndPrint();
    delay(autoReadPauseMillis);
  }


  if (Serial.available()) {
    char serialIn = Serial.read();

    if (serialIn == 'm') {
      autoRead = false;
      Serial.println("Setting to manual");
    } else if (serialIn == 'a') {
      autoRead = true;
      Serial.println("Setting to auto");
    }

    if (!autoRead && serialIn == 'r') {
      readSensorAndPrint();
    }
  }
}

int sum = 0;
int sumI = 0;
int maxSumI = 20;
float avgAnalogVal = 0.0;

void readSensorAndPrint() {
  int analogVal = analogRead(PS_ANALOG_PIN);

  if (isnan(analogVal)) {
    Serial.println("Failed to read from Pressure sensor");
    return;
  }

  sum += analogVal;
  sumI++;

  if (sumI == maxSumI) {
    avgAnalogVal = (float) sum / maxSumI;
    sum = 0;
    sumI = 0;
  }

  float quotientVal = (float) analogVal / MAX_ANALOG_VAL;
  float voltVal = quotientVal * 5.0;
  float height = calcHeight(analogVal);
  float avgHeight = calcHeight(avgAnalogVal);
  
  printReadingsToSerial(analogVal, quotientVal, voltVal, height, avgHeight);
  printReadingsToLcd(analogVal, voltVal, height, avgHeight);
}

// For real time values
float calcHeight(int analogVal) {
    // // Alte Messung
    // return 2.466 * analogVal - 245.1;

    // // Alle Werte, angepasst
    // return 2.499 * analogVal - 258.2;

    // Meterband Werte, angepasst
    return 2.487 * analogVal - 252.8;
}

// For avg values
float calcHeight(float analogVal) {
    // // Alte Messung
    // return 2.466 * analogVal - 245.1;

    // // Alle Werte, angepasst
    // return 2.499 * analogVal - 258.2;

    // Meterband Werte, angepasst
    return 2.487 * analogVal - 252.8;
}

void printReadingsToSerial(int analogVal, float quotientVal, float voltVal, float height, float avgHeight) {
  String modeText = "Mode: ";
  if (autoRead) {
    modeText += "auto";
  } else {
    modeText += "manual";
  }
  modeText += " Pin: " + String(PS_ANALOG_PIN - 14);
  Serial.println(modeText);

  Serial.print("Analog Value: ");
  Serial.println(analogVal);

  Serial.print("Quotient Value: ");
  Serial.println(quotientVal, 3);

  Serial.print("Voltage Value: ");
  Serial.print(voltVal, 3);
  Serial.println("V");

  Serial.print("Height: ");
  Serial.print(height, 3);
  Serial.println("cm");

  Serial.print("Average height: ");
  Serial.print(avgHeight, 3);
  Serial.println("cm");

  Serial.println();
}

void printReadingsToLcd(int analogVal, float voltVal, float height, float avgHeight) {
  lcd.clear();

  String modeText = "Mode: ";
  if (autoRead) {
    modeText += "auto";
  } else {
    modeText += "manual";
  }
  modeText += " Pin: " + String(PS_ANALOG_PIN - 14);
  lcd.setCursor(0, 0);
  lcd.print(modeText);

  String aText = "Analog: " + String(analogVal);
  lcd.setCursor(0, 1);
  lcd.print(aText);

  // String vText = "Voltage: " + String(voltVal, 1) + "V";
  // lcd.setCursor(0, 3);
  // lcd.print(vText);

  String hText = "Height: " + String(height, 2) + "cm";
  lcd.setCursor(0, 2);
  lcd.print(hText);

  String vhText = "Avg Height: " + String(avgHeight, 2) + "cm";
  lcd.setCursor(0, 3);
  lcd.print(vhText);
}
