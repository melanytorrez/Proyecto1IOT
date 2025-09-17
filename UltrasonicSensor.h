#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H
 
#include <Arduino.h>

class UltrasonicSensor {
public:
    // Rango por defecto para HC-SR04: 2 cm a 300 cm
    explicit UltrasonicSensor(uint8_t trigPin, uint8_t echoPin,
                              uint16_t minCm = 2, uint16_t maxCm = 300)
    : _trig(trigPin), _echo(echoPin), _minCm(minCm), _maxCm(maxCm) {
        // Velocidad del sonido por defecto a ~20°C: 343 m/s => 0.0343 cm/us
        _soundCmPerUs = 0.0343f;
        recomputeTimeout();
    }

    // Configura pines (llama en setup)
    void begin() {
        pinMode(_trig, OUTPUT);
        pinMode(_echo, INPUT);
        digitalWrite(_trig, LOW);
    }

    // Cambia rango válido (para recorte y timeout)
    void setRange(uint16_t minCm, uint16_t maxCm) {
        _minCm = minCm;
        _maxCm = maxCm;
        recomputeTimeout();
    }

    // (Opcional) Ajusta velocidad del sonido si quieres compensar temperatura/humedad.
    // Ejemplo: v(m/s) ≈ 331.3 + 0.606*T(°C). Convierte a cm/us dividiendo por 1e4.
    void setSoundSpeedCmPerUs(float cmPerUs) {
        if (cmPerUs > 0.02f && cmPerUs < 0.05f) { // chequeo básico
            _soundCmPerUs = cmPerUs;
        }
    }

    // Lee una distancia (cm). Devuelve NAN si no hay eco o fuera de rango.
    float readOnceCm() {
        // Pulso de disparo
        digitalWrite(_trig, LOW);
        delayMicroseconds(2);
        digitalWrite(_trig, HIGH);
        delayMicroseconds(10);
        digitalWrite(_trig, LOW);

        // Lee duración del eco (HIGH) con timeout acorde al rango
        unsigned long durUs = pulseIn(_echo, HIGH, _timeoutUs);
        if (durUs == 0) {
            return NAN; // timeout
        }

        // Distancia = (duración/2) * velocidad (ida y vuelta)
        float cm = (durUs * 0.5f) * _soundCmPerUs;

        // Recorta a rango válido
        if (cm < _minCm || cm > _maxCm) return NAN;
        return cm;
    }

    // Lee varias muestras y promedia las válidas (simple y efectivo contra ruido).
    // samples=1..10 recomendable. Si todas fallan, devuelve NAN.
    float getDistanceCm(uint8_t samples = 1, uint16_t interSampleDelayMs = 10) {
        if (samples < 1) samples = 1;
        if (samples > 10) samples = 10;

        float acc = 0.0f;
        uint8_t ok = 0;

        for (uint8_t i = 0; i < samples; ++i) {
            float d = readOnceCm();
            if (!isnan(d)) {
                acc += d;
                ok++;
            }
            if (i + 1 < samples && interSampleDelayMs > 0) {
                delay(interSampleDelayMs); // breve pausa entre disparos
            }
        }

        if (ok == 0) return NAN;
        return acc / ok;
    }

    // Accesores
    uint16_t minRangeCm() const { return _minCm; }
    uint16_t maxRangeCm() const { return _maxCm; }
    unsigned long timeoutUs() const { return _timeoutUs; }

private:
    uint8_t _trig, _echo;
    uint16_t _minCm, _maxCm;
    float _soundCmPerUs;       // ~0.0343 por defecto
    unsigned long _timeoutUs;  // deriva de maxRange

    void recomputeTimeout() {
        // Tiempo ida+vuelta (us) ≈ (2 * distancia_cm) / (cm/us)
        float maxRoundTripUs = (2.0f * (float)_maxCm) / _soundCmPerUs;
        // margen extra por seguridad
        _timeoutUs = (unsigned long)(maxRoundTripUs * 1.2f); // +20%
        // límite superior razonable (evitar bloqueos prolongados)
        if (_timeoutUs > 30000UL) _timeoutUs = 30000UL; // 30 ms
    }
};

#endif
