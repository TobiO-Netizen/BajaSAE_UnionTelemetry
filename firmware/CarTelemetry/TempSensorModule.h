#ifndef TEMP_SENSOR_MODULE_H
#define TEMP_SENSOR_MODULE_H

#include <Arduino.h>
#include <Wire.h>

// ============================================================
//  GY-906 / MLX90614 Infrared Temperature Sensor
//  — Ambient and object temperatures
//  — Callable in Fahrenheit, Celsius, or Kelvin
// ============================================================

enum TempUnit {
    CELSIUS,
    FAHRENHEIT,
    KELVIN
};

class TempSensorModule {
public:
    // Initialise I2C communication; returns true on success
    bool begin(uint8_t i2cAddr = 0x5A);

    // Update readings (call once per loop tick)
    void update();

    // ── Object (IR target) temperature ──────────────────────
    float getObjectTemp(TempUnit unit = CELSIUS) const;
    float getObjectTempC() const;
    float getObjectTempF() const;
    float getObjectTempK() const;

    // ── Ambient (die) temperature ───────────────────────────
    float getAmbientTemp(TempUnit unit = CELSIUS) const;
    float getAmbientTempC() const;
    float getAmbientTempF() const;
    float getAmbientTempK() const;

private:
    uint8_t _addr = 0x5A;
    float   _objectC  = 0;
    float   _ambientC = 0;

    float readRawTemp(uint8_t reg);

    static float cToF(float c) { return c * 9.0f / 5.0f + 32.0f; }
    static float cToK(float c) { return c + 273.15f; }
    float convert(float celsius, TempUnit unit) const;
};

#endif // TEMP_SENSOR_MODULE_H
