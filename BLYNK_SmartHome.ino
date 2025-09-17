#define BLYNK_TEMPLATE_ID "BLYNK_ID_NAME" // BLYNK_ID_NAME
#define BLYNK_TEMPLATE_NAME "BLYNK_TEMPLATE_NAME" // BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN "BLYNK_AUTH_TOKEN" // BLYNK_AUTH_TOKEN

#define BLYNK_PRINT Serial
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_LINES 2
#define DHTPIN 13
#define DHTTYPE DHT11
#define PIN_34 34

#include <Wire.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

float temp = 0;
float hum = 0;
float tempSum = 0;
int tempCount = 0;
float avgMinTemp = 100;
float avgMaxTemp = -100;
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "WIFI_NAME"; //WIFI_NAME
char pass[] = "WIFI_PASS"; //WIFI_PASS
const char* scriptURL = "https://script.google.com/macros/s/AKfycby-wC929uNjSS2D5xK_-v5KiSyniQBJJ3FCU5Q5dTo629PPcUx45Rr9MO1R0_tizDW-/exec"; // SCRIPT GOOGLE SHEETS

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Disconnected. Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
    }
}

void clearEEPROM() { // CLEAR EEPROM MEMORY
    for (int i = 0; i < 16; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared!");
}

void sendToExcel() { // SEND DATA TO GOOGLE SHEETS
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(scriptURL) + "?temp=" + temperature + "&hum=" + humidity;
        http.begin(url);
        http.GET();
        http.end();
    }
}

void sendSensor() { // SENSOR READ VALUE
  for (int j = 1; j < 6; j++) {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    
    if (isnan(temp) || isnan(hum)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    
    tempSum += temp;
    tempCount++;
    if (tempCount >= 50) {
        float avgTemp = tempSum / tempCount;
        
        if (avgTemp < avgMinTemp) {
            avgMinTemp = avgTemp;
            EEPROM.put(0, avgMinTemp);
        }
        if (avgTemp > avgMaxTemp) {
            avgMaxTemp = avgTemp;
            EEPROM.put(4, avgMaxTemp);
        }
        EEPROM.commit();
        tempSum = 0;
        tempCount = 0;
    }
    display_sensor(temp, hum, avgMinTemp, avgMaxTemp);
  }
}

void display_sensor(float &temp,float &hum,float &avgMinTemp,float &avgMaxTemp) { // DISPLAY VALUE TO LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(temp); lcd.print("C H:"); lcd.print(hum); lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("AN:"); lcd.print(avgMinTemp); lcd.print("AX:"); lcd.print(avgMaxTemp);
    
    Blynk.virtualWrite(V1, temp);
    Blynk.virtualWrite(V2, hum);
    delay(1000);
}

void humidity_in_missouri() { //ตรวจจับค่าความชื้นดิน
    int humidity_missouri = analogRead(PIN_34);
    int prese_humidity_missouri = 100 - (humidity_missouri * 100) / 4095; //แปลงค่าความชื้นแบบ analog เปลี่ยนเป็น 0 - 100
    Blynk.virtualWrite(V4, prese_humidity_missouri);
}

void sendTime() { // DISPLAY TIME TO LCD
  for (int i = 1; i < 6; i++) {
    DateTime now = rtc.now();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Date: ");
    lcd.print(now.year());
    lcd.print("/");
    printTwoDigits(now.month());
    lcd.print("/");
    printTwoDigits(now.day());

    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    printTwoDigits(now.hour());
    lcd.print(":");
    printTwoDigits(now.minute());
    lcd.print(":");
    printTwoDigits(now.second());
    delay(1000);
  }
}

void printTwoDigits(int number) { // DISPLAY DIGITS NUMBER TO LCD
    if (number < 10) lcd.print("0");
    lcd.print(number);
}

void setup() { // SETUP ARDUINO
    EEPROM.begin(16);
    Serial.begin(9600);
    Wire.begin();
    lcd.init();
    lcd.backlight();
    dht.begin();
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        lcd.setCursor(0, 0);
        lcd.print(".");
        delay(500);
    }
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("WiFi Connected!");
    delay(1000);
    lcd.clear();
    
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        abort();
    }
    if (!rtc.isrunning()) {
        Serial.println("RTC is NOT running, setting the time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    EEPROM.get(0, avgMinTemp);
    EEPROM.get(4, avgMaxTemp);
    if (isnan(avgMinTemp) || avgMinTemp == 0) avgMinTemp = 100; // กำหนดค่าเริ่มต้นที่สูง
    if (isnan(avgMaxTemp) || avgMaxTemp == 0) avgMaxTemp = -100; // กำหนดค่าเริ่มต้นที่ต่ำ
    
    Blynk.begin(auth, ssid, pass);
    timer.setInterval(1000L, humidity_in_missouri);
    timer.setInterval(5000L, sendTime);
    timer.setInterval(5000L, sendSensor);
    timer.setInterval(5000L, sendToExcel);
}

void loop() { // LOOP FUNCTION
    checkWiFiConnection();
    Blynk.run();
    timer.run();
}
