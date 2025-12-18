// ESP32_SensorHub_ThresholdColorMapped_v2.ino
// Maps delta = (pixel - ambient) to 0–255 so that:
//  - ΔT < 1.0°C       -> blue shades
//  - 1.0°C <= ΔT < 2.0°C -> greenish / yellowish region
//  - ΔT >= 2.0°C      -> red shades, from light red (2°C) to dark red (~3°C+)

#include <Wire.h>
#include <Adafruit_AMG88xx.h>

Adafruit_AMG88xx amg;

// Pins (ESP32)
const int SDA_PIN       = 21;
const int SCL_PIN       = 22;
const int LIDAR_RX      = 16; // TF-Mini TX -> ESP RX2
const int LIDAR_TX      = 17; // TF-Mini RX -> ESP TX2
const int PIR_LEFT_PIN  = 34;
const int PIR_RIGHT_PIN = 35;
const int SERIAL1_RX    = 14; // ESP32 UART1 RX -> UNO TX
const int SERIAL1_TX    = 18; // ESP32 UART1 TX -> UNO RX

HardwareSerial lidarSerial(2); // UART2 for TF-Mini

// AMG buffers
float raw_pixels[64];
float smooth_pixels[64];

// timing
unsigned long lastSample = 0;
const int SAMPLE_MS = 100;  // sensor sampling period

// LiDAR
int lidarDistance = 0;

// PIR
struct PIRState {
  int pin;
  bool state;
};

PIRState pirL, pirR;

void pirInit(PIRState &p, int pin) {
  p.pin = pin;
  pinMode(pin, INPUT);
  p.state = false;
}

void pirUpdate(PIRState &p) {
  p.state = (digitalRead(p.pin) == HIGH);
}

// 3x3 spatial smoothing to make heatmap less noisy
void spatialSmooth(float *in, float *out) {
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      float sum = 0;
      int count = 0;
      for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
          int rr = r + dr;
          int cc = c + dc;
          if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
            sum += in[rr * 8 + cc];
            count++;
          }
        }
      }
      out[r * 8 + c] = sum / count;
    }
  }
}

// TFmini reader (one frame, non-blocking-ish)
bool readTFminiFrame(int &dist, int &strength) {
  const unsigned long start = millis();
  while (millis() - start < 100) {
    if (lidarSerial.available() >= 9) {
      if ((uint8_t)lidarSerial.peek() != 0x59) {
        lidarSerial.read();
        continue;
      }
      uint8_t h1 = lidarSerial.read();
      if (lidarSerial.available() < 8) return false;
      uint8_t h2 = lidarSerial.read();
      if (h1 == 0x59 && h2 == 0x59) {
        uint8_t buf[7];
        for (int i = 0; i < 7; i++) buf[i] = lidarSerial.read();
        uint16_t d = buf[0] + (buf[1] << 8);
        uint16_t s = buf[2] + (buf[3] << 8);
        uint8_t cs = buf[6];
        uint8_t calc = 0;
        calc += 0x59;
        calc += 0x59;
        for (int i = 0; i < 6; i++) calc += buf[i];
        calc %= 256;
        if (calc == cs) {
          dist = d;
          strength = s;
          return true;
        }
      }
    }
  }
  return false;
}

// --------- NEW: delta -> 0..255 mapping with your required behaviour ----------
uint8_t mapDeltaToClass(float delta) {
  // Any negative or tiny delta -> deep/cool blue
  if (delta <= 0.0f) {
    return 20; // dark blue
  }

  // 0 < delta < 1.0°C: slightly warmer than ambient -> brighter blue / cyan
  if (delta < 1.0f) {
    // map 0..1  ->  40..80
    float t = delta / 1.0f; // 0..1
    return (uint8_t)(40 + t * 40); // 40..80
  }

  // 1.0°C <= delta < 2.0°C: warm region -> greenish / yellowish on your gradient
  if (delta < 2.0f) {
    // map 1..2 ->  110..170
    float t = (delta - 1.0f) / 1.0f; // 0..1
    return (uint8_t)(110 + t * 60); // 110..170
  }

  // delta >= 2.0°C: must be RED (human).
  // 2.0..3.0°C -> lighter red to darkest red
  float dClamped = delta;
  if (dClamped > 3.0f) dClamped = 3.0f;
  float t = (dClamped - 2.0f) / 1.0f;  // 0..1
  return (uint8_t)(200 + t * 55);      // 200..255 (all red region)
}

// Build CSV line: "CSV:b0,b1,...,b63;DIST:x;L:y;R:z"
String buildCSVline() {
  // 1) Compute ambient from smoothed pixels
  float ambient = 0.0f;
  for (int i = 0; i < 64; i++) ambient += smooth_pixels[i];
  ambient /= 64.0f;

  // 2) Build pixel part: 64 mapped bytes
  String out = "CSV:";

  for (int i = 0; i < 64; i++) {
    float delta = smooth_pixels[i] - ambient;
    uint8_t b = mapDeltaToClass(delta);
    out += b;
    if (i < 63) out += ",";
  }

  // 3) Append distance + PIR flags
  out += ";DIST:";
  out += String(lidarDistance);
  out += ";L:";
  out += String(pirL.state ? 1 : 0);
  out += ";R:";
  out += String(pirR.state ? 1 : 0);

  return out;
}

void sampleSensors() {
  if (millis() - lastSample >= SAMPLE_MS) {
    lastSample = millis();

    // AMG -> smooth
    amg.readPixels(raw_pixels);
    spatialSmooth(raw_pixels, smooth_pixels);

    // LiDAR
    int d, s;
    if (readTFminiFrame(d, s)) {
      lidarDistance = d;
    }

    // PIRs
    pirUpdate(pirL);
    pirUpdate(pirR);
  }
}

void setup() {
  Serial.begin(115200); // for debug
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RX, SERIAL1_TX); // to UNO
  lidarSerial.begin(115200, SERIAL_8N1, LIDAR_RX, LIDAR_TX);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(200);

  if (!amg.begin()) {
    Serial.println("AMG8833 not found! Check wiring.");
    while (1) delay(500);
  }

  pirInit(pirL, PIR_LEFT_PIN);
  pirInit(pirR, PIR_RIGHT_PIN);

  Serial.println("ESP32 Sensor Hub ready with delta-based RED threshold >= 2°C.");
}

void loop() {
  sampleSensors();

  // Respond to UNO requests
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == 'R') {
      String line = buildCSVline();
      Serial1.println(line);
      Serial1.flush();

      // Optional debug:
      Serial.println(line);
    }
  }

  delay(2);
}
