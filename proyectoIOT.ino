#include "Led.h"
#include "Buzzer.h"
#include "UltrasonicSensor.h"
#include "WarningSystem.h"
 
// ==== Pines (ejemplo ESP32) ====
constexpr uint8_t PIN_TRIG   = 4;
constexpr uint8_t PIN_ECHO   = 5;
constexpr uint8_t PIN_RED    = 12;
constexpr uint8_t PIN_ORANGE = 14;
constexpr uint8_t PIN_GREEN  = 27;
constexpr uint8_t PIN_BUZZER = 26;

// ==== Objetos ====
UltrasonicSensor sensor(PIN_TRIG, PIN_ECHO, 2, 300);
Led redLed(PIN_RED), orangeLed(PIN_ORANGE), greenLed(PIN_GREEN);
Buzzer buzzer(PIN_BUZZER);
WarningSystem ws(sensor, redLed, orangeLed, greenLed, buzzer);

void setup() {
  Serial.begin(115200);
  sensor.begin();

  ws.setThresholds(50, 100);   // rojo <50, naranja [50,100), verde >=100
  ws.setHysteresis(5);         // ±5 cm
  ws.setPollPeriodMs(50);      // 20 Hz

  WarningSystem::BuzzerProfile bzR{10.0f, 0.50f};
  WarningSystem::BuzzerProfile bzO{ 4.0f, 0.30f};
  WarningSystem::BuzzerProfile bzG{ 1.0f, 0.10f};
  ws.setBuzzerProfiles(bzR, bzO, bzG);

  ws.begin();
}

void loop() {
  ws.update();

  static unsigned long t0 = 0;
  if (millis() - t0 > 250) {
    t0 = millis();
    float d = ws.lastDistanceCm();
    Serial.print("d = ");
    if (isnan(d)) Serial.print("NaN");
    else          Serial.print(d, 1);
    Serial.print(" cm | zone = ");
    Serial.println(ws.zone());
  }
}

