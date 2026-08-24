#ifndef MAIN_SYSTEM_H
#define MAIN_SYSTEM_H

#include <Arduino.h>
#include "Config.h"
#include "MPU6050Module.h"
#include "LoRaModule.h"
#include "SDCardModule.h"
#include "TempSensorModule.h"
#include "GPSModule.h"
#include "StrainGaugeModule.h"
#include "OBD2Module.h"

// ============================================================
//  Main System — orchestrates every subsystem
//  — Initialises all modules and reports status
//  — Runs a timed telemetry loop
//  — Builds CSV log lines and LoRa packets
// ============================================================

class MainSystem {
public:
    // Call in setup()  — returns true if ALL modules initialised OK
    bool begin();

    // Call in loop()   — handles timing, reads sensors, logs & transmits
    void update();

    // ── Direct access to sub-modules if needed externally ───
    MPU6050Module      mpu;
    LoRaModule         lora;
    SDCardModule       sd;
    TempSensorModule   temp;
    GPSModule          gps;
    StrainGaugeModule  strain;
    OBD2Module         obd;

private:
    // Non-blocking timers
    unsigned long _lastSensorMs  = 0;
    unsigned long _lastLogMs     = 0;
    unsigned long _lastLoraMs    = 0;

    uint32_t _packetCount = 0;
    bool     _obdConnected = false;

    // Build a CSV row from current sensor state
    String buildCSVRow();

    // Build a compact LoRa payload
    String buildLoRaPayload();

    void readAllSensors();
    void logToSD();
    void transmitLoRa();
};

#endif // MAIN_SYSTEM_H
