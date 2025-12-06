/*
Uploading working perfectly
WiFi working perfectly
GPS location acquiring working perfectly
OLED display arranged with custom pins ✅
WiFi status, Bus Tracker, Date/Time, Lat/Lon handled
*/

#include <TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

// ---------------- GPS Object ----------------
TinyGPSPlus gps;

// ---------------- Pin Definitions ----------------
#define GPS_RX_PIN 33  // GPS TX → ESP32 RX
#define GPS_TX_PIN 32  // GPS RX → ESP32 TX
#define uploadLED 12
#define GPSLED 13
#define buzzer 14

// ---------------- OLED Custom Pins ----------------
// SDA = GPIO4, SCL = GPIO15
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0, 15, 4, U8X8_PIN_NONE); // clock, data, reset

// ---------------- Wi-Fi Credentials ----------------
const char* ssid = "Mamun";
const char* password = "mamun1234";

// ---------------- Supabase API Details ----------------
const char* supabaseUrl = "https://gzjbszsodhhucwusyizz.supabase.co";
const char* supabaseApiKey = "Your API here ";
const char* tableName = "tracker1"; // Table name in Supabase

// ---------------- Hardware Serial for GPS ----------------
HardwareSerial mySerial(2); // UART2

// ---------------- Function Prototypes ----------------
void setupPins();
void connectToWiFi();
void signalBuzzer();
void shortbip();
void displayData();
void readGPSData();
void uploadToSupabase(float latitude, float longitude);
void blinkIndicator(int pin);

// ---------------- Setup Pins ----------------
void setupPins() {
    pinMode(uploadLED, OUTPUT);
    pinMode(GPSLED, OUTPUT);
    pinMode(buzzer, OUTPUT);
}

// ---------------- Wi-Fi Connection ----------------
void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    // OLED will show WiFi status in loop()
}

// ---------------- Buzzer Functions ----------------
void signalBuzzer() {
    digitalWrite(buzzer, HIGH);
    delay(200);
    digitalWrite(buzzer, LOW);
}

void shortbip() {
    digitalWrite(buzzer, HIGH);
    delay(10);
    digitalWrite(buzzer, LOW);
}

// ---------------- OLED Display Function ----------------
void displayData() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    if (WiFi.status() == WL_CONNECTED) {
        // ------------------- WiFi Connected -------------------
        u8g2.setCursor(0, 10);
        u8g2.print("Diu Bus Tracker");  // উপরের লেখা

        // ------------------- Date & Time -------------------
        if (gps.date.isValid() && gps.time.isValid()) {
            u8g2.setCursor(0, 20);
            u8g2.printf("Date: %02d/%02d/%04d", gps.date.day(), gps.date.month(), gps.date.year());

            u8g2.setCursor(0, 30);
            u8g2.printf("Time: %02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
        } else {
            u8g2.setCursor(0, 20);
            u8g2.print("Date: --/--/----");
            u8g2.setCursor(0, 30);
            u8g2.print("Time: --:--:--");
        }

        // ------------------- GPS Lat/Lon -------------------
        if (gps.location.isValid()) {
            u8g2.setCursor(0, 45);
            u8g2.printf("Lat: %.6f", gps.location.lat());
            u8g2.setCursor(0, 55);
            u8g2.printf("Lon: %.6f", gps.location.lng());
        } else {
            u8g2.setCursor(0, 45);
            u8g2.print("Lat: --");
            u8g2.setCursor(0, 55);
            u8g2.print("Lon: --");
        }

    } else {
        // ------------------- WiFi Disconnected -------------------
        u8g2.setCursor(0, 10);
        u8g2.print("Bus Tracker: 🚍🚍");  // Top bus tracker

        u8g2.setCursor(0, 20);
        u8g2.print("WiFi: Disconnected");

        u8g2.setCursor(0, 30);
        u8g2.print("Trying to connect;)");
        u8g2.setCursor(0, 40);
        u8g2.print("Sorry <3");

        // Auto reconnect
        WiFi.begin(ssid, password);
    }

    u8g2.sendBuffer();
}

// ---------------- GPS Data ----------------
void readGPSData() {
    while (mySerial.available() > 0) {
        gps.encode(mySerial.read());

        if (gps.location.isUpdated() && WiFi.status() == WL_CONNECTED) {
            float latitude = gps.location.lat();
            float longitude = gps.location.lng();

            String mapLink = "https://www.google.com/maps/search/?api=1&query=" 
                             + String(latitude, 6) + "," + String(longitude, 6);

            Serial.print("\nLatitude: "); Serial.println(latitude, 6);
            Serial.print("Longitude: "); Serial.println(longitude, 6);
            Serial.print("Google Maps Link: "); Serial.println(mapLink);

            uploadToSupabase(latitude, longitude);
            blinkIndicator(GPSLED);
        }
    }
}

// ---------------- Upload to Supabase ----------------
void uploadToSupabase(float latitude, float longitude) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String requestUrl = String(supabaseUrl) + "/rest/v1/" + tableName;

        StaticJsonDocument<200> jsonDoc;
        jsonDoc["latitude"] = latitude;
        jsonDoc["longitude"] = longitude;
        jsonDoc["mapLink"] = "https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);

        String jsonPayload;
        serializeJson(jsonDoc, jsonPayload);

        Serial.println("Uploading to: " + requestUrl);
        Serial.println("Payload: " + jsonPayload);

        http.begin(requestUrl);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + String(supabaseApiKey));
        http.addHeader("apikey", String(supabaseApiKey));
        http.addHeader("Prefer", "return=minimal");

        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            Serial.println("Data uploaded successfully. Response code: " + String(httpResponseCode));
            blinkIndicator(uploadLED);
            shortbip();
        } else {
            Serial.println("Failed to upload data. HTTP Error: " + String(httpResponseCode));
        }

        http.end();
    }
}

// ---------------- LED Blink ----------------
void blinkIndicator(int pin) {
    digitalWrite(pin, HIGH);
    delay(15);
    digitalWrite(pin, LOW);
}

// ---------------- Setup ----------------
void setup() {
    Serial.begin(115200);
    setupPins();
    u8g2.begin();
    mySerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); // UART2 with pins 33,32
    connectToWiFi();
}

// ---------------- Loop ----------------
void loop() {
    readGPSData();
    displayData(); // OLED refresh every loop
    delay(1000);
}
