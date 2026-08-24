#ifndef OBD2_MODULE_H
#define OBD2_MODULE_H

#include <Arduino.h>
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ============================================================
//  OBD2 Module  (CAN bus via ESP32 TWAI)
//  — Auto-detects if OBD2 is plugged in
//  — Background polling on core 0
//  — Thread-safe cache for RPM, speed, coolant temp
// ============================================================

struct OBDData {
    float    rpm;
    float    speedKph;
    float    speedMph;
    float    coolantC;       // Celsius
    float    coolantF;       // Fahrenheit
    float    throttle;       // percent 0-100

    uint32_t rpm_ts;         // millis() when last updated
    uint32_t speed_ts;
    uint32_t coolant_ts;
    uint32_t throttle_ts;
};

class OBD2Module {
public:
    // Initialize TWAI driver; returns true if CAN bus starts
    bool begin(gpio_num_t txPin, gpio_num_t rxPin);

    // Stop CAN driver and background task
    void stop();

    // Probe ECU with PID 0x00 — returns true if OBD2 responds
    bool isConnected();

    // Start background polling task on core 0
    void startPolling();

    // Thread-safe snapshot of all cached data
    OBDData getSnapshot();

    // Individual accessors (thread-safe)
    float getRPM();
    float getSpeedMph();
    float getSpeedKph();
    float getCoolantC();
    float getCoolantF();
    float getThrottle();

    // Is a reading still fresh (within maxAgeMs)?
    bool isFresh(uint32_t timestamp, uint32_t maxAgeMs = 500);

    bool isPolling() const;

private:
    static constexpr uint8_t PID_RPM      = 0x0C;
    static constexpr uint8_t PID_SPEED    = 0x0D;
    static constexpr uint8_t PID_THROTTLE = 0x11;
    static constexpr uint8_t PID_COOLANT  = 0x05;

    volatile OBDData    _cache = {};
    SemaphoreHandle_t   _mutex = nullptr;
    TaskHandle_t        _taskHandle = nullptr;
    bool                _polling = false;
    bool                _driverInstalled = false;

    bool requestPID(uint8_t pid);
    bool readResponse(uint8_t expectedPid, uint8_t *out, int *outLen,
                      uint32_t timeoutMs = 80);
    void updateCache(uint8_t pid, const uint8_t *data);
    void pollOne(uint8_t pid);

    static void obdTaskWrapper(void *param);
    void obdTaskLoop();
};

#endif // OBD2_MODULE_H
