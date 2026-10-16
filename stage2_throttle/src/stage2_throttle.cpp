// Stage 2 - Throttle pedal simulation with a potentiometer.
// The red/green LEDs are blended with PWM according to the pot position, and a
// semicircular gauge needle on the OLED shows the throttle percentage.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int POT_PIN = 34; // ADC1_CH6, input only
const int RED_PIN = 25;
const int GRN_PIN = 26;

// LEDC API (Core 2.x). On 3.x analogWrite is enough.
const int PWM_FREQ = 5000;
const int PWM_RES = 8;
const int RED_CHANNEL = 0;
const int GRN_CHANNEL = 1;

const float SMOOTH_ALPHA = 0.2f; // EMA: new = a*raw + (1-a)*old
const int DEADZONE = 80;         // the ADC does not read exactly 0 with the pot at zero
const long UI_INTERVAL = 50;     // 20 FPS
const long LOG_INTERVAL = 250;

// Gauge geometry: pivot at the bottom centre of the screen, arc spans 180..360 degrees
const int CX = 64;
const int CY = 62;
const int R_OUT = 42;
const int R_TICK_IN = 38;
const int R_NEEDLE = 40;

float smoothedAdc = 0.0f;
unsigned long lastUiMs = 0;
unsigned long lastLogMs = 0;

void writeLeds(int pct);
void throttleToRG(int pct, uint8_t &r, uint8_t &g);
const char *zoneName(int pct);
void drawDashboard(int pct, int adc);

void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\n--- Stage2: Throttle Simulation ---");

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db); // full 0..3.3V range

  ledcSetup(RED_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(RED_PIN, RED_CHANNEL);
  ledcSetup(GRN_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(GRN_PIN, GRN_CHANNEL);
  ledcWrite(RED_CHANNEL, 0);
  ledcWrite(GRN_CHANNEL, 0);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) // default address of the 128x64 SSD1306
  {
    Serial.println(F("OLED init failed!"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("STAGE 2 - THROTTLE");
  display.setCursor(28, 40);
  display.println("Initializing...");
  display.display();
  delay(1000);

  // Seed the EMA with the first reading, otherwise it ramps up from zero.
  smoothedAdc = analogRead(POT_PIN);
  Serial.println("Ready.");
}

void loop()
{
  int raw = analogRead(POT_PIN);
  smoothedAdc = SMOOTH_ALPHA * raw + (1.0f - SMOOTH_ALPHA) * smoothedAdc;
  int adc = (int)smoothedAdc;

  if (adc < DEADZONE)
    adc = 0;

  int pct = map(adc, 0, 4095, 0, 100);
  pct = constrain(pct, 0, 100);

  writeLeds(pct);

  unsigned long now = millis();

  if (now - lastUiMs >= UI_INTERVAL)
  {
    lastUiMs = now;
    drawDashboard(pct, adc);
  }

  if (now - lastLogMs >= LOG_INTERVAL)
  {
    lastLogMs = now;
    Serial.print("ADC:");
    Serial.print(adc);
    Serial.print("  PCT:");
    Serial.print(pct);
    Serial.print("%  Zone:");
    Serial.println(zoneName(pct));
  }
}

// 0% -> pure green, 50% -> yellow, 100% -> pure red
void throttleToRG(int pct, uint8_t &r, uint8_t &g)
{
  if (pct <= 50)
  {
    r = map(pct, 0, 50, 0, 255);
    g = 255;
  }
  else
  {
    r = 255;
    g = map(pct, 50, 100, 255, 0);
  }
}

void writeLeds(int pct)
{
  uint8_t r, g;
  throttleToRG(pct, r, g);
  ledcWrite(RED_CHANNEL, r);
  ledcWrite(GRN_CHANNEL, g);
}

const char *zoneName(int pct)
{
  if (pct < 34)
    return "LOW";
  if (pct < 67)
    return "MID";
  return "HIGH";
}

void drawDashboard(int pct, int adc)
{
  (void)adc; // reserved for a future debug overlay
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("THROTTLE");

  char hdr[10];
  snprintf(hdr, sizeof(hdr), "%3d%% %s", pct, zoneName(pct));

  // GFX default font is 6 px wide -> right-align
  int hdrLen = strlen(hdr);
  display.setCursor(SCREEN_WIDTH - hdrLen * 6, 0);
  display.print(hdr);

  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

  // Outer arc: join points in 3-degree steps with line segments
  int prevX = -1, prevY = -1;
  for (int deg = 180; deg <= 360; deg += 3)
  {
    float rad = deg * (float)PI / 180.0f;
    int x = CX + (int)(R_OUT * cosf(rad));
    int y = CY + (int)(R_OUT * sinf(rad));
    if (prevX >= 0)
      display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
    prevX = x;
    prevY = y;
  }

  // Ticks at 0/25/50/75/100; 0, 50 and 100 are longer.
  for (int t = 0; t <= 100; t += 25)
  {
    float deg = 180.0f + (t * 180.0f / 100.0f);
    float rad = deg * (float)PI / 180.0f;
    int rIn = (t % 50 == 0) ? R_TICK_IN - 3 : R_TICK_IN;

    int x1 = CX + (int)(rIn * cosf(rad));
    int y1 = CY + (int)(rIn * sinf(rad));
    int x2 = CX + (int)(R_OUT * cosf(rad));
    int y2 = CY + (int)(R_OUT * sinf(rad));
    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
  }

  display.setCursor(CX - R_OUT - 6, CY - 7);
  display.print("0");
  display.setCursor(CX + R_OUT - 10, CY - 7);
  display.print("100");

  // Needle
  float ndeg = 180.0f + (pct * 180.0f / 100.0f);
  float nrad = ndeg * (float)PI / 180.0f;
  int nx = CX + (int)(R_NEEDLE * cosf(nrad));
  int ny = CY + (int)(R_NEEDLE * sinf(nrad));
  display.drawLine(CX, CY, nx, ny, SSD1306_WHITE);

  // Pivot
  display.fillCircle(CX, CY, 3, SSD1306_WHITE);
  display.drawCircle(CX, CY, 5, SSD1306_WHITE);

  display.display();
}
