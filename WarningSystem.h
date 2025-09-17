#ifndef WARNING_SYSTEM_H
#define WARNING_SYSTEM_H
 
#include <Arduino.h>
#include "UltrasonicSensor.h"
#include "Led.h"
#include "Buzzer.h"

class WarningSystem {
public:
    enum Zone : uint8_t { Z_NONE = 0, Z_RED, Z_ORANGE, Z_GREEN };

    struct BuzzerProfile {
        float freqHz;   // pitidos por segundo (1 = 1 pitido/segundo)
        float duty;     // fracción encendida (0.0–1.0)
    };

    WarningSystem(UltrasonicSensor& sensor,
                  Led& red, Led& orange, Led& green,
                  Buzzer& buzzer)
    : _sensor(sensor), _red(red), _orange(orange), _green(green), _buzzer(buzzer) {}

    // Llamar desde setup() si quieres ajustar valores antes de empezar
    void begin() {
        // nada obligatorio aquí; los componentes ya están inicializados
        applyZone(Z_NONE); // todo apagado hasta primera lectura válida
        _lastEvalMs = millis();
    }

    // Actualiza el sistema (no bloqueante). Llamar MUY seguido en loop().
    void update() {
        // 1) Mantener patrones de parpadeo/buzzer sin bloquear
        _red .update();
        _orange.update();
        _green.update();
        _buzzer.update();

        // 2) ¿Toca muestrear distancia?
        unsigned long now = millis();
        if (now - _lastEvalMs < _pollPeriodMs) return;
        _lastEvalMs = now;

        // 3) Leer distancia (1 muestra rápida para latencia baja)
        float d = _sensor.getDistanceCm(1, 0);
        _lastDistance = d;

        // 4) Decidir zona con histéresis
        Zone newZone = decideZoneWithHysteresis(d);

        // 5) Si cambió la zona, aplicar LEDs + buzzer
        if (newZone != _zone) {
            applyZone(newZone);
            _zone = newZone;
        }
    }

    // -------- Configuración --------
    void setThresholds(uint16_t nearCm, uint16_t midCm) {
        _thNear = nearCm;
        _thMid  = midCm;
    }

    void setHysteresis(uint8_t hysteresisCm) {
        _hys = hysteresisCm;
    }

    void setPollPeriodMs(uint16_t periodMs) {
        _pollPeriodMs = periodMs; // p.ej. 50 ms => 20 Hz
        if (_pollPeriodMs < 10) _pollPeriodMs = 10; // evita sobrecargar
    }

    void setBuzzerProfiles(const BuzzerProfile& red,
                           const BuzzerProfile& orange,
                           const BuzzerProfile& green) {
        _bzRed = red; _bzOrange = orange; _bzGreen = green;
    }

    // -------- Lecturas / estado --------
    float    lastDistanceCm() const { return _lastDistance; }
    Zone     zone() const { return _zone; }
    uint16_t nearThreshold() const { return _thNear; }
    uint16_t midThreshold()  const { return _thMid;  }

private:
    UltrasonicSensor& _sensor;
    Led& _red;
    Led& _orange;
    Led& _green;
    Buzzer& _buzzer;

    // Config por defecto (puedes cambiarlos en setup())
    uint16_t _thNear = 50;   // < 50 cm => ROJO
    uint16_t _thMid  = 100;  // [50,100) => NARANJA ; >=100 => VERDE
    uint8_t  _hys    = 5;    // ±5 cm histéresis
    uint16_t _pollPeriodMs = 50; // 20 Hz decisión

    // Perfiles de buzzer por zona (freq y duty)
    BuzzerProfile _bzRed    { 10.0f, 0.50f }; // muy repetitivo
    BuzzerProfile _bzOrange {  4.0f, 0.30f }; // más frecuente
    BuzzerProfile _bzGreen  {  1.0f, 0.10f }; // espaciado

    // Estado
    Zone _zone = Z_NONE;
    float _lastDistance = NAN;
    unsigned long _lastEvalMs = 0;

    Zone decideZoneWithHysteresis(float d) {
        // Si no hay lectura válida: fuera de rango => Z_NONE
        if (isnan(d)) return Z_NONE;

        // Bandas con histéresis según la zona actual
        switch (_zone) {
            case Z_RED:
                // permanece en rojo hasta que supere (near + hys)
                if (d >= _thNear + _hys) {
                    // pasa a naranja si está por debajo de mid - hys
                    if (d < _thMid - _hys) return Z_ORANGE;
                    else return Z_GREEN; // salto directo si se va lejos
                }
                return Z_RED;

            case Z_ORANGE:
                // Si baja por debajo de (near - hys) => rojo
                if (d < _thNear - _hys) return Z_RED;
                // Si sube por encima de (mid + hys) => verde
                if (d >= _thMid + _hys) return Z_GREEN;
                return Z_ORANGE;

            case Z_GREEN:
                // permanece verde hasta bajar de (mid - hys)
                if (d < _thMid - _hys) {
                    // si cae por debajo de near + hys podría ir naranja/rojo
                    if (d < _thNear + _hys) return Z_RED;
                    return Z_ORANGE;
                }
                return Z_GREEN;

            case Z_NONE:
            default:
                // Clasificación inicial sin histéresis
                if (d < _thNear) return Z_RED;
                if (d < _thMid)  return Z_ORANGE;
                return Z_GREEN;
        }
    }

    void applyZone(Zone z) {
        // LEDs
        if (z == Z_RED) {
            _red.blink(5.0f);
            _orange.turnOff();
            _green.turnOff();
            // Buzzer
            _buzzer.setPattern(_bzRed.freqHz, _bzRed.duty);
        }
        else if (z == Z_ORANGE) {
            _red.turnOff();
            _orange.blink(5.0f);
            _green.turnOff();
            _buzzer.setPattern(_bzOrange.freqHz, _bzOrange.duty);
        }
        else if (z == Z_GREEN) {
            _red.turnOff();
            _orange.turnOff();
            _green.turnOn();
            _buzzer.setPattern(_bzGreen.freqHz, _bzGreen.duty);
        }
        else { // Z_NONE (fuera de rango / sin eco)
            _red.turnOff();
            _orange.turnOff();
            _green.turnOff();
            _buzzer.mute();
        }
    }
};

#endif
