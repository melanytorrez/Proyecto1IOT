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
  // Mantener sistema en tiempo real (LEDs + buzzer)
  ws.update();

  // --- LOG periódico de estado ---
  static unsigned long tLog = 0;
  if (millis() - tLog >= 500) { // cada 500 ms
    tLog = millis();

    float d = ws.lastDistanceCm();
    uint8_t z = ws.zone();
    unsigned long tr = ws.lastReactionMs();

    Serial.print("Distancia = ");
    if (isnan(d)) Serial.print("NaN");
    else Serial.printf("%.1f cm", d);

    Serial.print(" | Zona = ");
    switch (z) {
      case WarningSystem::Z_RED:    Serial.print("Rojo"); break;
      case WarningSystem::Z_ORANGE: Serial.print("Naranja"); break;
      case WarningSystem::Z_GREEN:  Serial.print("Verde"); break;
      case WarningSystem::Z_NONE:   Serial.print("None"); break;
    }

    Serial.print(" | Reaction_ms = ");
    Serial.println(tr);
  }
}

