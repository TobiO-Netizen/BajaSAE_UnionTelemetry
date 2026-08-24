#include "TempSensorModule.h"

// MLX90614 RAM registers
#define MLX_TA   0x06   // ambient
#define MLX_TOBJ 0x07   // object

bool TempSensorModule::begin(uint8_t i2cAddr) {
    _addr = i2cAddr;
    Wire.begin();      // safe to call multiple times

    // Probe the sensor with a dummy read
    Wire.beginTransmission(_addr);
    uint8_t err = Wire.endTransmission();
    return (err == 0);
}

void TempSensorModule::update() {
    _ambientC = readRawTemp(MLX_TA);
    _objectC  = readRawTemp(MLX_TOBJ);
}

// ── object temps ─────────────────────────────────────────────
float TempSensorModule::getObjectTemp(TempUnit unit)  const { return convert(_objectC, unit); }
float TempSensorModule::getObjectTempC()              const { return _objectC; }
float TempSensorModule::getObjectTempF()              const { return cToF(_objectC); }
float TempSensorModule::getObjectTempK()              const { return cToK(_objectC); }

// ── ambient temps ────────────────────────────────────────────
float TempSensorModule::getAmbientTemp(TempUnit unit) const { return convert(_ambientC, unit); }
float TempSensorModule::getAmbientTempC()             const { return _ambientC; }
float TempSensorModule::getAmbientTempF()             const { return cToF(_ambientC); }
float TempSensorModule::getAmbientTempK()             const { return cToK(_ambientC); }

// ── internals ────────────────────────────────────────────────
float TempSensorModule::readRawTemp(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(_addr, (uint8_t)3);   // 2 data + 1 PEC
    if (Wire.available() < 2) return -999.0f;

    uint16_t raw = Wire.read();
    raw |= (Wire.read() << 8);
    Wire.read();   // discard PEC byte

    // Convert to °C  (MLX formula: T = raw * 0.02 − 273.15)
    return raw * 0.02f - 273.15f;
}

float TempSensorModule::convert(float celsius, TempUnit unit) const {
    switch (unit) {
        case FAHRENHEIT: return cToF(celsius);
        case KELVIN:     return cToK(celsius);
        default:         return celsius;
    }
}
