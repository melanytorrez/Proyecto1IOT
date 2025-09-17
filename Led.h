#ifndef LED_H
#define LED_H
 
#include <Arduino.h>

class Led {
public:
    enum State : uint8_t { OFF = 0, ON = 1, BLINK = 2 };

    explicit Led(uint8_t pin) {
        _pin = pin;
        pinMode(_pin, OUTPUT);
        writeOff();
        _state = OFF;
    }

    // Encender fijo
    void turnOn() { setState(ON); }

    // Apagar fijo
    void turnOff() { setState(OFF); }

    // Activar parpadeo a cierta frecuencia (Hz)
    void blink(float hz = 5.0f) {
        setBlinkHz(hz);
        setState(BLINK);
    }

    // Debe llamarse frecuentemente en loop()
    void update() {
        if (_state != BLINK) return;

        unsigned long now = millis();
        if (now - _prevMs >= _toggleIntervalMs) {
            _prevMs = now;
            if (_isOn) writeOff();
            else       writeOn();
        }
    }

    // Configura frecuencia de parpadeo
    void setBlinkHz(float hz) {
        if (hz < 0.5f)  hz = 0.5f;   // mínimo
        if (hz > 50.0f) hz = 50.0f;  // máximo
        _toggleIntervalMs = (unsigned long)(500.0f / hz); // medio ciclo
    }

    // Saber estado actual
    State state() const { return _state; }
    bool  isOn()  const { return _isOn; }

private:
    uint8_t _pin;
    State   _state;
    bool    _isOn = false;
    unsigned long _toggleIntervalMs = 100; // por defecto = 5 Hz
    unsigned long _prevMs = 0;

    void writeOn()  { digitalWrite(_pin, HIGH); _isOn = true; }
    void writeOff() { digitalWrite(_pin, LOW);  _isOn = false; }

    void setState(State s) {
        _state = s;
        if (s == OFF) {
            writeOff();
        } else if (s == ON) {
            writeOn();
        } else if (s == BLINK) {
            _prevMs = millis();
            writeOff(); // comienza apagado para que el primer cambio sea ON
        }
    }
};

#endif
