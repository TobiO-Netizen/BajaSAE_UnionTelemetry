#ifndef STRAIN_GAUGE_MODULE_H
#define STRAIN_GAUGE_MODULE_H

#include <Arduino.h>

// ============================================================
//  Strain Gauge Module  (350Ω gauge + HX711 24-bit ADC)
//  — Tare / zero offset
//  — Configurable calibration factor
//  — Force in Newtons, lbf, and raw counts
//  — Selectable gain (128 or 64 on channel A, 32 on channel B)
// ============================================================

class StrainGaugeModule {
public:
    // Initialise HX711; returns true if the chip responds
    bool begin(uint8_t doutPin, uint8_t sckPin, uint8_t gain = 128);

    // Call once per loop tick — reads a new sample if available
    void update();

    // ── Calibration ─────────────────────────────────────────
    // 1. Place a known weight on the gauge
    // 2. Read getRawAvg() with that weight
    // 3. calibrationFactor = (rawWithWeight − rawZero) / knownForceN
    // 4. Call setCalibrationFactor() with that value
    void tare(uint8_t samples = 20);                 // zero with no load
    void setCalibrationFactor(float factor);          // counts per Newton
    float getCalibrationFactor() const;

    // ── Readings ────────────────────────────────────────────
    float getForceN()   const;   // Newtons
    float getForceLbf() const;   // pound-force
    float getForceKg()  const;   // kilogram-force (mass shorthand)
    long  getRaw()      const;   // raw 24-bit count (after tare)
    long  getRawAvg(uint8_t samples = 10);  // averaged raw for calibration

    // ── Status ──────────────────────────────────────────────
    bool isReady() const;        // HX711 has a new sample

private:
    uint8_t _doutPin = 0;
    uint8_t _sckPin  = 0;
    uint8_t _gain    = 128;

    long  _offset    = 0;        // tare offset (raw counts at zero load)
    float _calFactor = 1.0f;    // counts per Newton
    long  _lastRaw   = 0;       // most recent raw reading minus offset

    // Bit-bang one 24-bit conversion from HX711
    long readRaw();

    // Set gain for next conversion (128/64 ch-A, 32 ch-B)
    uint8_t gainPulses() const;
};

#endif // STRAIN_GAUGE_MODULE_H
