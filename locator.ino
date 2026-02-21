#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FSR_PIN A0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found! Check wiring.");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WaterBuddy ready!");
  display.display();
  delay(1500);
}

void loop() {
  int fsrRaw = analogRead(FSR_PIN);

  // Convert raw 0-1023 to a rough percentage
  int percent = map(fsrRaw, 0, 1023, 0, 100);

  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("-- WaterBuddy --");

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

  delay(150);
}