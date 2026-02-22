#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define FSR_PIN 34

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ESP32 = 12 bit, range 0-4095

  Wire.begin(21, 22); // SDA=21, SCL=22
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("WaterBuddy ready!");
  display.display();
  delay(1500);
}

void loop() {
  int fsrRaw = analogRead(FSR_PIN);
  int percent = map(fsrRaw, 0, 4095, 0, 100);

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("-- WaterBuddy --");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("Raw: ");
  display.println(fsrRaw);

  display.setTextSize(2);
  display.setCursor(0, 36);
  display.print("Pct: ");
  display.print(percent);
  display.println("%");

  // Bar graph
  display.drawRect(0, 56, 128, 8, WHITE);
  int barWidth = map(fsrRaw, 0, 4095, 0, 126);
  display.fillRect(1, 57, barWidth, 6, WHITE);

  display.display();

  Serial.print("FSR Raw: ");
  Serial.print(fsrRaw);
  Serial.print("  |  Percent: ");
  Serial.println(percent);

  delay(150);
}