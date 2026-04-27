#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ========== WiFi + ThingSpeak Config ==========
const char* ssid = "SRC";          // your Wi-Fi name
const char* password = "src@internet"; // your Wi-Fi password
String apiKey = "ZXA0SSBU6YA4ZKS7";    // your ThingSpeak Write API Key
const char* server = "http://api.thingspeak.com/update";

// ========== LCD ==========
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== Sensors ==========
#define ONE_WIRE_BUS 32
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MAX30105 particleSensor;

// ========== Buzzer ==========
#define BUZZER 14

// ========== Variables ==========
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;
float temperatureC;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  sensors.begin();

  // Start MAX30105
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    lcd.clear();
    lcd.print("MAX30105 Error!");
    Serial.println("MAX30105 not found. Check wiring/power.");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Connect Wi-Fi
  lcd.clear();
  lcd.print("WiFi Connecting");
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    lcd.print(".");
    retry++;
  }
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected");
    Serial.println("WiFi connected!");
  } else {
    lcd.print("WiFi Failed");
    Serial.println("WiFi failed!");
  }

  delay(2000);
  lcd.clear();
  lcd.print("Place Finger...");
}

void loop() {
  // Wait for finger
  while (particleSensor.getIR() < 50000) {
    lcd.clear();
    lcd.print("No Finger");
    delay(300);
  }

  lcd.clear();
  lcd.print("Measuring...");
  long startTime = millis();

  while (millis() - startTime < 20000) {
    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  }

  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HR:");
  lcd.print(beatAvg);
  lcd.print(" BPM");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(temperatureC, 1);
  lcd.print(" C");

  // Alert
  if (beatAvg > 100 || temperatureC > 38) {
    digitalWrite(BUZZER, HIGH);
    delay(2000);
    digitalWrite(BUZZER, LOW);
  }

  // === Upload to ThingSpeak ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = server;
    url += "?api_key=" + apiKey;
    url += "&field1=" + String(beatAvg);
    url += "&field2=" + String(temperatureC, 1);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.println("ThingSpeak updated. Code: " + String(httpCode));
    } else {
      Serial.println("Error sending data: " + http.errorToString(httpCode));
    }

    http.end();
  } else {
    Serial.println("WiFi not connected. Skipping upload.");
  }

  delay(15000); // ThingSpeak minimum 15 sec update limit
}