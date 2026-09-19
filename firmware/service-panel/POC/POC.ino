#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Arduino Nano (ATmega328P) I2C: A4 = SDA, A5 = SCL.
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr int8_t OLED_RESET = -1;  // Most 4-pin I2C modules have no reset pin.

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 40 x 28 pixel boot emblem. PROGMEM keeps all 140 bytes in flash, not SRAM.
const uint8_t PROGMEM NO_GHOST_BITMAP[] = {
  0x00, 0x02, 0x02, 0xF0, 0x00, 0x00, 0x00, 0x82, 0x1C, 0x00,
  0x00, 0x0C, 0x02, 0x07, 0x00, 0x00, 0x18, 0x03, 0xC1, 0x80,
  0x00, 0x30, 0x01, 0xF0, 0xC0, 0x00, 0x70, 0x01, 0xFC, 0x60,
  0x00, 0x70, 0x40, 0x1C, 0x20, 0x00, 0x90, 0x00, 0x0C, 0x30,
  0x00, 0xB0, 0x00, 0x30, 0x10, 0x01, 0x60, 0x00, 0xC1, 0xF0,
  0x41, 0xC0, 0x03, 0x87, 0xC0, 0x49, 0x80, 0x06, 0x1C, 0x08,
  0x03, 0x00, 0x18, 0x30, 0x04, 0x80, 0x00, 0x70, 0xC0, 0x01,
  0x00, 0x0D, 0xC3, 0x80, 0x48, 0x00, 0x3F, 0x0E, 0x0C, 0xF4,
  0x00, 0xFC, 0x18, 0x00, 0x98, 0x01, 0x40, 0x60, 0x01, 0x98,
  0x00, 0xC1, 0xC0, 0x01, 0x10, 0x00, 0x83, 0xD0, 0x03, 0x30,
  0x00, 0x43, 0x80, 0x06, 0x20, 0x00, 0x63, 0x00, 0x0C, 0x40,
  0x00, 0x30, 0xC0, 0x70, 0x80, 0x00, 0x18, 0x3F, 0xC1, 0x80,
  0x00, 0x0E, 0x00, 0x06, 0x00, 0x00, 0x03, 0x80, 0x1C, 0x00,
  0x00, 0x00, 0xF9, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void centerFlashString(const __FlashStringHelper *text, uint8_t y) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.getTextBounds(text, 0, y, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, y);
  display.print(text);
}

void drawNoGhostIcon() {
  // Rows 16..43 stay entirely within the blue region on dual-color OLEDs.
  display.drawBitmap(44, 16, NO_GHOST_BITMAP, 40, 28, SSD1306_WHITE);
}

void showBootFrame(const __FlashStringHelper *message, uint8_t progress) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  centerFlashString(F("PROTON PACK"), 4);  // Yellow band on dual-color OLEDs.
  drawNoGhostIcon();                       // Blue band on dual-color OLEDs.
  centerFlashString(message, 45);

  display.drawRect(13, 54, 102, 8, SSD1306_WHITE);
  display.fillRect(16, 57, progress, 2, SSD1306_WHITE);  // progress: 0..96
  display.display();
}

void runBootSequence() {
  showBootFrame(F("INITIALIZING"), 8);
  delay(350);
  showBootFrame(F("CHECKING WAND"), 38);
  delay(350);
  showBootFrame(F("CHARGING CELLS"), 68);
  delay(350);
  showBootFrame(F("SYSTEM READY"), 96);
  delay(500);
}

void drawStatusScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Header and panel dividers.
  display.fillRect(0, 0, SCREEN_WIDTH, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(13, 2);
  display.print(F("SERVICE PANEL // 84"));

  display.setTextColor(SSD1306_WHITE);
  display.drawLine(0, 27, 127, 27, SSD1306_WHITE);
  display.drawLine(0, 44, 127, 44, SSD1306_WHITE);
  display.drawLine(83, 45, 83, 63, SSD1306_WHITE);

  // Fixed labels and values stay in flash through F().
  display.setCursor(3, 16);
  display.print(F("PACK:"));
  display.setCursor(54, 16);
  display.print(F("ONLINE"));
  display.fillCircle(119, 19, 3, SSD1306_WHITE);

  display.setCursor(3, 33);
  display.print(F("WAND:"));
  display.setCursor(54, 33);
  display.print(F("ONLINE"));
  display.fillCircle(119, 36, 3, SSD1306_WHITE);

  display.setCursor(3, 51);
  display.print(F("BATTERY:"));
  display.setCursor(52, 51);
  display.print(F("11.8V"));

  display.setCursor(88, 48);
  display.print(F("AUDIO"));
  display.setCursor(93, 57);
  display.print(F("70%"));

  display.display();
}

void setup() {
  Wire.begin();

  // SSD1306_SWITCHCAPVCC is correct for common self-powered 128x64 modules.
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // No serial strings or retry buffer: remain safely halted if OLED init fails.
    for (;;) {
      delay(1000);
    }
  }

  display.setTextWrap(false);
  runBootSequence();
  drawStatusScreen();
}

void loop() {
  // Static proof-of-concept screen; not integrated with the pack firmware.
}
