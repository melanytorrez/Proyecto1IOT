#ifndef BUZZER_H
#define BUZZER_H
 
#include <Arduino.h>

class Buzzer {
public:
    enum State : uint8_t { OFF = 0, ON = 1, PATTERN = 2 };

    explicit Buzzer(uint8_t pin) {
        _pin = pin;
        pinMode(_pin, OUTPUT);
        writeOff();
        _state = OFF;
    }

    // Encender fijo (tono continuo)
    void turnOn() { 
        setState(ON); 
    }

    // Apagar completamente
    void turnOff() { 
        setState(OFF); 
    }

    // Configurar un patrón de pitidos
    // freqHz: pitidos por segundo (ej: 1 = cada 1 s, 10 = muy rápido)
    // duty: fracción del ciclo encendido (0.0–1.0), ej: 0.5 = 50%
    void setPattern(float freqHz, float duty = 0.5f) {
        if (freqHz < 0.5f) freqHz = 0.5f;
        if (freqHz > 50.0f) freqHz = 50.0f;
        if (duty < 0.05f) duty = 0.05f;
        if (duty > 0.95f) duty = 0.95f;

        _toggleIntervalOn  = (unsigned long)(1000.0f * duty / freqHz);
        _toggleIntervalOff = (unsigned long)(1000.0f * (1.0f - duty) / freqHz);

        setState(PATTERN);
        _prevMs = millis();
        writeOn(); // comienza encendido
    }

    // Mute (alias para turnOff)
    void mute() { 
        turnOff(); 
    }

    // Debe llamarse en cada loop()
    void update() {
        if (_state != PATTERN) return;

        unsigned long now = millis();

        if (_isOn) {
            if (now - _prevMs >= _toggleIntervalOn) {
                _prevMs = now;
                writeOff();
            }
        } else {
            if (now - _prevMs >= _toggleIntervalOff) {
                _prevMs = now;
                writeOn();
            }
        }
    }

    // Estado actual
    State state() const { return _state; }
    bool  isOn()  const { return _isOn; }

private:
    uint8_t _pin;
    State   _state;
    bool    _isOn = false;

    unsigned long _toggleIntervalOn  = 500; // ms encendido
    unsigned long _toggleIntervalOff = 500; // ms apagado
    unsigned long _prevMs = 0;

    void writeOn()  { digitalWrite(_pin, HIGH); _isOn = true; }
    void writeOff() { digitalWrite(_pin, LOW);  _isOn = false; }

    void setState(State s) {
        _state = s;
        if (s == OFF) {
            writeOff();
        } else if (s == ON) {
            writeOn();
        } else if (s == PATTERN) {
            _prevMs = millis();
        }
    }
};

#endif
