#ifndef MPU6050_MODULE_H
#define MPU6050_MODULE_H

#include <Arduino.h>
#include <Wire.h>

// ============================================================
//  MPU6050 Module
//  — Raw & filtered acceleration (g)
//  — Gyroscope rates (°/s)
// ============================================================

struct AccelData {
    float x;   // g
    float y;
    float z;
};

struct GyroData {
    float x;   // °/s
    float y;
    float z;
};

struct MPU6050Reading {
    AccelData accel;
    GyroData  gyro;
};

class MPU6050Module {
public:
    // Initialise sensor; returns true on success
    bool begin();

    // Call once per loop iteration — updates internal state
    void update();

    // Latest processed reading
    const MPU6050Reading& getReading() const;

    // Convenience accessors
    AccelData getAccel() const;
    GyroData  getGyro()  const;

    // Calibrate at rest — averages N samples to compute offsets
    void calibrate(uint16_t samples = 200);

    // Car-setting helpers
    void setAccelRange(uint8_t range);   // 0=±2g, 1=±4g, 2=±8g, 3=±16g
    void setGyroRange(uint8_t range);    // 0=±250, 1=±500, 2=±1000, 3=±2000 °/s

private:
    static constexpr uint8_t MPU_ADDR = 0x68;

    MPU6050Reading _reading = {};
    float _axOff = 0, _ayOff = 0, _azOff = 0;
    float _gxOff = 0, _gyOff = 0, _gzOff = 0;
    float _accelScale = 16384.0f;   // default +/-2 g
    float _gyroScale  = 131.0f;     // default +/-250 deg/s

    void writeRegister(uint8_t reg, uint8_t value);
    int16_t read16(uint8_t regHigh);
};

#endif // MPU6050_MODULE_H
