#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FSR_PIN A0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "eduroam";
const char* password = "YOUR_WIFI_PASSWORD";
const char* firebaseURL = "https://nemooo-default-rtdb.firebaseio.com/";

float totalIntake_mL = 0;          // Total water consumed in mL
float sip_mL = 0;                  // Amount drunk in last sip
float goal_mL = 1000;              // Your daily goal (example 1000 mL)

void setup() {
  Serial.begin(9600);

  // Wifi Setup
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  //FSR Setup

  
  // Display Setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found! Check wiring.");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Nemo ready!");
  display.display();
  delay(1500);
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to switch screens
  if (currentMillis - lastSwitch >= switchInterval) {
    showFirstScreen = !showFirstScreen;  // toggle screen
    lastSwitch = currentMillis;
  }

  int fsrRaw = analogRead(FSR_PIN);

  // Convert raw 0-1023 to a rough percentage
  int percent = map(fsrRaw, 0, 1023, 0, 100);

  display.clearDisplay();

  if (showFirstScreen) {
  // --- First screen ---
    // Title
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("-- Nemo --");
    display.println("-- The WaterBuddy --");
  
    // Big raw number
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print("Raw: ");
    display.println(fsrRaw);
  
    // Percent
    display.setTextSize(2);
    display.setCursor(0, 36);
    display.print("Pct: ");
    display.print(percent);
    display.println("%");
  
    // Bar graph across the bottom
    display.drawRect(0, 56, 128, 8, WHITE);
    int barWidth = map(fsrRaw, 0, 1023, 0, 126);
    display.fillRect(1, 57, barWidth, 6, WHITE);
  
    display.display();
  
    // Also print to Serial Monitor for debugging
    Serial.print("FSR Raw: ");
    Serial.print(fsrRaw);
    Serial.print("  |  Percent: ");
    Serial.println(percent);
  else {
  // --- Second screen ---
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("-- Your Stats --");

    // Example: show total intake & percent goal
    float totalIntake_oz = fsrRaw / 29.5735; // example conversion
    int percentGoal = percent; // placeholder

    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print("Total oz:");
    display.println(totalIntake_oz, 1);

    display.setCursor(0, 36);
    display.print("% Goal:");
    display.println(percentGoal);
  }

  //Wifi Connection Loop
  
    
  delay(150);
}
