#include "StrainGaugeModule.h"

// ── Public API ───────────────────────────────────────────────
bool StrainGaugeModule::begin(uint8_t doutPin, uint8_t sckPin, uint8_t gain) {
    _doutPin = doutPin;
    _sckPin  = sckPin;
    _gain    = gain;

    pinMode(_sckPin, OUTPUT);
    pinMode(_doutPin, INPUT);

    // Power-cycle the HX711: pull SCK high >60us then low
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(100);
    digitalWrite(_sckPin, LOW);

    // Wait for first conversion (can take up to 400ms at 10 SPS)
    unsigned long start = millis();
    while (!isReady()) {
        if (millis() - start > 500) return false;   // chip not responding
    }

    // Perform one throwaway read to set the gain
    readRaw();

    return true;
}

void StrainGaugeModule::update() {
    if (!isReady()) return;
    _lastRaw = readRaw() - _offset;
}

// ── Calibration ──────────────────────────────────────────────
void StrainGaugeModule::tare(uint8_t samples) {
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        while (!isReady()) { delay(1); }
        sum += readRaw();
    }
    _offset  = sum / samples;
    _lastRaw = 0;
}

void  StrainGaugeModule::setCalibrationFactor(float factor) { _calFactor = factor; }
float StrainGaugeModule::getCalibrationFactor() const       { return _calFactor; }

// ── Readings ─────────────────────────────────────────────────
float StrainGaugeModule::getForceN()   const { return _lastRaw / _calFactor; }
float StrainGaugeModule::getForceLbf() const { return getForceN() * 0.224809f; }
float StrainGaugeModule::getForceKg()  const { return getForceN() / 9.80665f; }
long  StrainGaugeModule::getRaw()      const { return _lastRaw; }

long StrainGaugeModule::getRawAvg(uint8_t samples) {
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        while (!isReady()) { delay(1); }
        sum += readRaw() - _offset;
    }
    return sum / samples;
}

// ── Status ───────────────────────────────────────────────────
bool StrainGaugeModule::isReady() const {
    // HX711 pulls DOUT low when a new conversion is ready
    return digitalRead(_doutPin) == LOW;
}

// ── Internals ────────────────────────────────────────────────
long StrainGaugeModule::readRaw() {
    // Bit-bang 24 data bits (MSB first) + gain-setting pulses
    long value = 0;

    // 24 clock pulses for 24 bits of data
    for (uint8_t i = 0; i < 24; i++) {
        digitalWrite(_sckPin, HIGH);
        delayMicroseconds(1);
        value = (value << 1) | digitalRead(_doutPin);
        digitalWrite(_sckPin, LOW);
        delayMicroseconds(1);
    }

    // Extra pulses to set gain for NEXT conversion
    uint8_t extra = gainPulses();
    for (uint8_t i = 0; i < extra; i++) {
        digitalWrite(_sckPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(_sckPin, LOW);
        delayMicroseconds(1);
    }

    // Convert 24-bit two's complement to signed long
    if (value & 0x800000) {
        value |= 0xFF000000;   // sign-extend to 32 bits
    }

    return value;
}

uint8_t StrainGaugeModule::gainPulses() const {
    // Total pulses = 24 data + extra.  25 total = gain 128 ch-A,
    // 26 = gain 32 ch-B, 27 = gain 64 ch-A
    switch (_gain) {
        case 32:  return 2;   // channel B
        case 64:  return 3;   // channel A, gain 64
        default:  return 1;   // channel A, gain 128
    }
}
