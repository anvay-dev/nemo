#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "esp_eap_client.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FSR_PIN 34
#define NUM_SAMPLES 10
#define ALERT_THRESHOLD 70
#define CALIBRATE_SAMPLES 50

// Eduroam credentials
const char* SSID     = "eduroam";
const char* IDENTITY = "#";  // your university email
const char* USERNAME = "#";  // your university email
const char* PASSWORD = "#";  // your university password

// Firebase credentials
#define FIREBASE_HOST "https://nemooo-a14ef-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "m0mwyKCO6FkBOptDJMSBahMG1LLdEthltT6nvJlM"

// Logging thresholds
#define LIFT_THRESHOLD 95
#define MIN_CHANGE_TO_LOG 3

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

int fsrReadings[NUM_SAMPLES];
int readIndex = 0;
int readingSum = 0;
int smoothedRaw = 0;

int fullBottleRaw = 0;
int emptyBottleRaw = 0;

int lastLoggedPercent = -1;
bool bottleWasLifted = false;

unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 200;

// --- Helpers ---

int stableRead(int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(FSR_PIN);
    delay(20);
  }
  return sum / samples;
}

String getTimestamp() {
  return String(millis());
}

void logToFirebase(int percent, String event) {
  String path = "/waterbuddy/logs/" + getTimestamp();

  FirebaseJson json;
  json.set("percent_drunk", percent);
  json.set("event", event);
  json.set("timestamp_ms", (int)millis());

  if (Firebase.setJSON(fbdo, path, json)) {
    Serial.println("Firebase log success: " + event + " | " + String(percent) + "%");
  } else {
    Serial.println("Firebase error: " + fbdo.errorReason());
  }

  FirebaseJson latest;
  latest.set("percent_drunk", percent);
  latest.set("event", event);
  latest.set("timestamp_ms", (int)millis());
  Firebase.setJSON(fbdo, "/waterbuddy/latest", latest);
}

// --- Calibration ---

void calibrate() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("CALIBRATION");
  display.setCursor(0, 25);
  display.println("Place FULL bottle");
  display.setCursor(0, 35);
  display.println("on sensor...");
  display.setCursor(0, 50);
  display.println("Waiting 5 seconds");
  display.display();
  delay(5000);

  fullBottleRaw = stableRead(CALIBRATE_SAMPLES);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("CALIBRATION");
  display.setCursor(0, 25);
  display.println("Now place EMPTY");
  display.setCursor(0, 35);
  display.println("bottle on sensor...");
  display.setCursor(0, 50);
  display.println("Waiting 5 seconds");
  display.display();
  delay(5000);

  emptyBottleRaw = stableRead(CALIBRATE_SAMPLES);

  Serial.print("Full raw: "); Serial.println(fullBottleRaw);
  Serial.print("Empty raw: "); Serial.println(emptyBottleRaw);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("Calibration done!");
  display.setCursor(10, 35);
  display.println("Connecting WiFi...");
  display.display();
}

// --- WiFi + Firebase Setup (eduroam) ---

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  Serial.println("Scanning for eduroam...");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == String(SSID)) {
      Serial.println("Found eduroam in scan.");
      break;
    }
  }

  esp_eap_client_set_ttls_phase2_method(ESP_EAP_TTLS_PHASE2_PAP);
  WiFi.begin(SSID, WPA2_AUTH_PEAP, IDENTITY, USERNAME, PASSWORD);

  Serial.print("Connecting to eduroam");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("WiFi connected!");
    display.setCursor(0, 35);
    display.println(WiFi.localIP().toString());
    display.display();
    delay(1500);
  } else {
    Serial.println("\nWiFi failed! Running offline.");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("WiFi FAILED.");
    display.setCursor(0, 35);
    display.println("Running offline.");
    display.display();
    delay(1500);
  }
}

void setupFirebase() {
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// --- Setup ---

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("OLED not found!");
      while (1);
    }
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(20, 20);
  display.println("Nemo!");
  display.setCursor(28, 36);
  display.println("Stay hydrated!");
  display.display();
  delay(1500);

  calibrate();
  connectWiFi();
  setupFirebase();

  int initialVal = analogRead(FSR_PIN);
  for (int i = 0; i < NUM_SAMPLES; i++) {
    fsrReadings[i] = initialVal;
  }
  readingSum = initialVal * NUM_SAMPLES;
  smoothedRaw = initialVal;
}

// --- Main Loop ---

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = now;

    int newReading = analogRead(FSR_PIN);
    readingSum -= fsrReadings[readIndex];
    fsrReadings[readIndex] = newReading;
    readingSum += newReading;
    readIndex = (readIndex + 1) % NUM_SAMPLES;
    smoothedRaw = readingSum / NUM_SAMPLES;

    int percentDrunk = constrain(
      map(smoothedRaw, fullBottleRaw, emptyBottleRaw, 0, 100),
      0, 100
    );

    int barWidth = constrain(map(percentDrunk, 0, 100, 0, 126), 0, 126);

    bool bottleLifted = (percentDrunk >= LIFT_THRESHOLD);

    if (bottleLifted) {
      if (!bottleWasLifted) {
        Serial.println("Bottle lifted — pausing logging.");
        bottleWasLifted = true;
      }
    } else {
      if (bottleWasLifted) {
        Serial.println("Bottle returned to sensor.");
        lastLoggedPercent = percentDrunk;
        bottleWasLifted = false;
      }

      if (lastLoggedPercent == -1) {
        lastLoggedPercent = percentDrunk;
        logToFirebase(percentDrunk, "baseline");
      } else {
        int delta = percentDrunk - lastLoggedPercent;
        if (abs(delta) >= MIN_CHANGE_TO_LOG) {
          String event;
          if (delta > 0) {
            event = "drank " + String(delta) + "% since last log";
          } else {
            event = "refilled " + String(abs(delta)) + "% since last log";
          }
          logToFirebase(percentDrunk, event);
          lastLoggedPercent = percentDrunk;
        }
      }
    }

    // --- Display ---
    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(22, 0);
    display.println("-- Nemo --");

    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print("Raw: ");
    display.print(newReading);
    display.print("  Avg: ");
    display.println(smoothedRaw);

    display.setTextSize(2);
    display.setCursor(0, 24);
    if (bottleLifted) {
      display.println("No bottle!");
    } else {
      display.print("Drunk: ");
      display.print(percentDrunk);
      display.println("%");
    }

    display.setTextSize(1);
    display.setCursor(0, 44);
    if (bottleLifted) {
      display.println("Place bottle down");
    } else if (percentDrunk >= ALERT_THRESHOLD) {
      display.println("!! REFILL SOON !!");
    } else if (percentDrunk >= 30) {
      display.println("Good progress!");
    } else {
      display.println("Bottle is full!");
    }

    display.drawRect(0, 55, 128, 9, WHITE);
    display.fillRect(1, 56, barWidth, 7, WHITE);
    display.display();

    Serial.print("FSR Raw: "); Serial.print(newReading);
    Serial.print(" | Smoothed: "); Serial.print(smoothedRaw);
    Serial.print(" | Drunk: "); Serial.print(percentDrunk);
    Serial.print("% | Lifted: "); Serial.println(bottleLifted ? "YES" : "NO");
  }
}