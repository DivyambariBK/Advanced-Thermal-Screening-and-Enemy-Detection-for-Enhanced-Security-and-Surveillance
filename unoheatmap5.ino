// UNO_Display_Interpolated.ino
// - Keeps layout: landscape, top heatmap, bottom status
// - Interpolates 8x8 sensor data to 24x24 tiles (smooth human shape)
// - Bytes >= 200 are forced to pure RED (human)

#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
MCUFRIEND_kbv tft;

uint8_t pixels[64];   // 0..255 from ESP32
int lastDist = 0;
int leftFlag = 0;
int rightFlag = 0;

unsigned long lastRequest = 0;
const unsigned long FRAME_INTERVAL_MS = 200;

int DISP_W = 320;
int DISP_H = 480;
int HEAT_H = 360;   // top 3/4 of screen
int STATUS_H = 120; // bottom 1/4 of screen

// interpolation factor: 8 * UPSCALE = output grid size
const int UPSCALE = 3; // 8x3 = 24x24 virtual tiles

// ----------------- COLOR HELPERS -----------------
uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// v is 0..255 (already encoded by ESP32)
// >= 200 means "human" → force RED
uint16_t tempToColor(uint8_t v) {
  if (v >= 200) {
    // confirmed human region -> strong red
    return rgbTo565(255, 0, 0);
  }

  float x = v / 255.0f;
  uint8_t r, g, b;

  if (x < 0.33f) {
    // dark blue -> cyan
    float t = x / 0.33f;
    r = 0;
    g = (uint8_t)(t * 140);
    b = (uint8_t)(180 + (1 - t) * 70);
  } else if (x < 0.66f) {
    // cyan/green -> yellow
    float t = (x - 0.33f) / 0.33f;
    r = (uint8_t)(t * 200);
    g = 200 + (uint8_t)(t * 55);
    b = (uint8_t)((1 - t) * 80);
  } else {
    // yellow -> orange (but NOT used for human; human is forced red above)
    float t = (x - 0.66f) / 0.34f;
    r = 220 + (uint8_t)(t * 35);
    g = 200 - (uint8_t)(t * 200);
    b = 0;
  }

  return rgbTo565(r, g, b);
}

// -------------- DRAW HEATMAP WITH INTERPOLATION --------------
void drawHeatmap() {
  const int SRC = 8;              // 8x8 original
  const int OUT = SRC * UPSCALE;  // e.g. 24x24 virtual grid

  int tileW = DISP_W / OUT;
  int tileH = HEAT_H / OUT;
  if (tileW < 1) tileW = 1;
  if (tileH < 1) tileH = 1;

  int totalW = tileW * OUT;
  int totalH = tileH * OUT;
  int startX = (DISP_W - totalW) / 2;
  int startY = (HEAT_H - totalH) / 2;

  // Bilinear interpolation from 8x8 -> OUT x OUT
  for (int oy = 0; oy < OUT; oy++) {
    float fy = (float)oy * (SRC - 1) / (float)(OUT - 1); // 0..7
    int y0 = (int)fy;
    float dy = fy - y0;
    int y1 = (y0 < SRC - 1) ? (y0 + 1) : y0;

    for (int ox = 0; ox < OUT; ox++) {
      float fx = (float)ox * (SRC - 1) / (float)(OUT - 1); // 0..7
      int x0 = (int)fx;
      float dx = fx - x0;
      int x1 = (x0 < SRC - 1) ? (x0 + 1) : x0;

      // read 4 neighbors
      uint8_t p00 = pixels[y0 * 8 + x0];
      uint8_t p10 = pixels[y0 * 8 + x1];
      uint8_t p01 = pixels[y1 * 8 + x0];
      uint8_t p11 = pixels[y1 * 8 + x1];

      // bilinear interpolation
      float vTop = p00 + dx * (p10 - p00);
      float vBot = p01 + dx * (p11 - p01);
      float v    = vTop + dy * (vBot - vTop);

      uint8_t val = (uint8_t) constrain((int)(v + 0.5f), 0, 255);
      uint16_t color = tempToColor(val);

      int x = startX + ox * tileW;
      int y = startY + oy * tileH;
      tft.fillRect(x, y, tileW, tileH, color);
    }
  }
}

// -------------- STATUS AREA (unchanged layout) --------------
void drawStatus() {
  int baseY = HEAT_H;
  tft.fillRect(0, baseY, DISP_W, STATUS_H, 0x0000);

  // Distance
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(3);
  tft.setCursor(10, baseY + 5);
  tft.print("DIST: ");
  tft.print(lastDist);
  tft.print(" cm");

  int pirY = baseY + 50;

  if (leftFlag && !rightFlag) {
    tft.setTextColor(rgbTo565(255, 0, 0), 0x0000);
    tft.setCursor(75, pirY);
    tft.print("TURN LEFT");
  } else if (rightFlag && !leftFlag) {
    tft.setTextColor(rgbTo565(255, 0, 0), 0x0000);
    tft.setCursor(65, pirY);
    tft.print("TURN RIGHT");
  } else {
    tft.setTextColor(rgbTo565(0, 255, 0), 0x0000);
    tft.setCursor(120, pirY);
    tft.print("CLEAR");
  }
}

// -------------- CSV PARSING -----------------
void parseCSVLine(String line) {
  line.trim();
  if (!line.startsWith("CSV:")) return;

  // strip "CSV:"
  line.remove(0, 4);

  int semiIndex = line.indexOf(';');
  if (semiIndex < 0) return;

  String csv = line.substring(0, semiIndex);
  int pixelIndex = 0;

  int last = 0;
  while (pixelIndex < 64 && last <= csv.length()) {
    int comma = csv.indexOf(',', last);
    String tok = (comma == -1) ? csv.substring(last) : csv.substring(last, comma);
    tok.trim();

    int val = tok.toInt();
    pixels[pixelIndex++] = (uint8_t) constrain(val, 0, 255);

    if (comma == -1) break;
    last = comma + 1;
  }

  // metadata
  int distPos = line.indexOf("DIST:");
  if (distPos >= 0) {
    int end = line.indexOf(';', distPos + 5);
    String d = (end >= 0) ? line.substring(distPos + 5, end) : line.substring(distPos + 5);
    d.trim();
    lastDist = d.toInt();
  }

  int lpos = line.indexOf("L:");
  if (lpos >= 0) {
    int end = line.indexOf(';', lpos + 2);
    String s = (end >= 0) ? line.substring(lpos + 2, end) : line.substring(lpos + 2);
    s.trim();
    leftFlag = s.toInt();
  }

  int rpos = line.indexOf("R:");
  if (rpos >= 0) {
    int end = line.indexOf(';', rpos + 2);
    String s = (end >= 0) ? line.substring(rpos + 2, end) : line.substring(rpos + 2);
    s.trim();
    rightFlag = s.toInt();
  }

  // update display
  drawHeatmap();
  drawStatus();
}

// -------------- READ ONE LINE FROM SERIAL --------------
String readLine(unsigned long timeoutMs) {
  unsigned long start = millis();
  String s = "";

  while (millis() - start < timeoutMs) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') return s;
      if (isPrintable(c)) s += c;
      start = millis();
    }
  }
  return s;
}

// ------------------ SETUP ----------------------
void setup() {
  Serial.begin(115200);

  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);   // landscape

  DISP_W = tft.width();
  DISP_H = tft.height();
  HEAT_H = (DISP_H * 3) / 4;
  STATUS_H = DISP_H - HEAT_H;

  tft.fillScreen(0x0000);

  // initial blank frame
  for (int i = 0; i < 64; i++) pixels[i] = 0;
  drawHeatmap();
  drawStatus();

  lastRequest = millis();
}

// ------------------- LOOP ----------------------
void loop() {
  unsigned long now = millis();

  // ask ESP32 for a new frame every FRAME_INTERVAL_MS
  if (now - lastRequest >= FRAME_INTERVAL_MS) {
    lastRequest = now;
    Serial.write('R');
  }

  // read and parse any incoming line
  String line = readLine(120);
  if (line.length() > 0) {
    parseCSVLine(line);
  }

  delay(2);
}
