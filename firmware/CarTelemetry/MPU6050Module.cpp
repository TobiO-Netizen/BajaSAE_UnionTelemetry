#include "MPU6050Module.h"

// -- Register map (subset) --
#define REG_PWR_MGMT_1   0x6B
#define REG_ACCEL_CONFIG  0x1C
#define REG_GYRO_CONFIG   0x1B
#define REG_ACCEL_XOUT_H  0x3B
#define REG_GYRO_XOUT_H   0x43

// -- helpers --
void MPU6050Module::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

int16_t MPU6050Module::read16(uint8_t regHigh) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(regHigh);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, (uint8_t)2);
    int16_t val = (Wire.read() << 8) | Wire.read();
    return val;
}

// -- public API --
bool MPU6050Module::begin() {
    Wire.begin();

    // Wake the sensor (clear sleep bit)
    writeRegister(REG_PWR_MGMT_1, 0x00);
    delay(100);

    // Verify identity
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x75); // WHO_AM_I
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, (uint8_t)1);
    uint8_t id = Wire.read();
    if (id != 0x68 && id != 0x72) {
        return false;
    }

    setAccelRange(0);   // +/-2 g
    setGyroRange(0);    // +/-250 deg/s
    return true;
}

void MPU6050Module::calibrate(uint16_t samples) {
    float ax = 0, ay = 0, az = 0;
    float gx = 0, gy = 0, gz = 0;

    for (uint16_t i = 0; i < samples; i++) {
        ax += read16(REG_ACCEL_XOUT_H) / _accelScale;
        ay += read16(REG_ACCEL_XOUT_H + 2) / _accelScale;
        az += read16(REG_ACCEL_XOUT_H + 4) / _accelScale;
        gx += read16(REG_GYRO_XOUT_H)     / _gyroScale;
        gy += read16(REG_GYRO_XOUT_H + 2)  / _gyroScale;
        gz += read16(REG_GYRO_XOUT_H + 4)  / _gyroScale;
        delay(3);
    }

    _axOff = ax / samples;
    _ayOff = ay / samples;
    _azOff = (az / samples) - 1.0f;   // gravity on Z when flat
    _gxOff = gx / samples;
    _gyOff = gy / samples;
    _gzOff = gz / samples;
}

void MPU6050Module::update() {
    float ax = read16(REG_ACCEL_XOUT_H)     / _accelScale - _axOff;
    float ay = read16(REG_ACCEL_XOUT_H + 2) / _accelScale - _ayOff;
    float az = read16(REG_ACCEL_XOUT_H + 4) / _accelScale - _azOff;

    float gx = read16(REG_GYRO_XOUT_H)     / _gyroScale - _gxOff;
    float gy = read16(REG_GYRO_XOUT_H + 2) / _gyroScale - _gyOff;
    float gz = read16(REG_GYRO_XOUT_H + 4) / _gyroScale - _gzOff;

    _reading.accel = { ax, ay, az };
    _reading.gyro  = { gx, gy, gz };
}

const MPU6050Reading& MPU6050Module::getReading() const { return _reading; }
AccelData MPU6050Module::getAccel() const { return _reading.accel; }
GyroData  MPU6050Module::getGyro()  const { return _reading.gyro; }

void MPU6050Module::setAccelRange(uint8_t range) {
    range = constrain(range, 0, 3);
    writeRegister(REG_ACCEL_CONFIG, range << 3);
    const float scales[] = { 16384.0f, 8192.0f, 4096.0f, 2048.0f };
    _accelScale = scales[range];
}

void MPU6050Module::setGyroRange(uint8_t range) {
    range = constrain(range, 0, 3);
    writeRegister(REG_GYRO_CONFIG, range << 3);
    const float scales[] = { 131.0f, 65.5f, 32.8f, 16.4f };
    _gyroScale = scales[range];
}
