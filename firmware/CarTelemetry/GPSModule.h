#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include "Config.h"

// ============================================================
//  GPS Module  (NEO-6M via UART / NMEA)
//  — Lightweight NMEA parser (no library dependency)
//  — Lat/Lon, altitude, speed, heading, satellite count
//  — UTC date & time
// ============================================================

struct GPSData {
    bool   hasFix;
    float  latitude;       // decimal degrees (+ N, − S)
    float  longitude;      // decimal degrees (+ E, − W)
    float  altitude;       // metres above MSL
    float  speedKnots;
    float  speedKph;
    float  speedMph;
    float  course;         // degrees true
    uint8_t satellites;

    // UTC time & date from RMC sentence
    uint8_t  hour, minute, second;
    uint8_t  day, month;
    uint16_t year;
};

class GPSModule {
public:
    // Starts the UART; returns true immediately (fix may take time)
    bool begin(HardwareSerial &serial,
               long baud  = GPS_BAUD,
               int  rxPin = GPS_RXD,
               int  txPin = GPS_TXD);

    // Feed bytes from the serial port — call every loop()
    void update();

    // Latest parsed fix
    const GPSData& getData() const;

    bool hasFix()   const;
    float latitude() const;
    float longitude() const;

    // Human-readable timestamp  "2025-04-15 18:30:45"
    String timestamp() const;

private:
    HardwareSerial *_serial = nullptr;
    GPSData _data = {};

    char   _buf[120];
    uint8_t _bufIdx = 0;

    void parseSentence(const char *sentence);
    void parseGGA(const char *s);
    void parseRMC(const char *s);

    static float nmeaToDecimal(const char *raw, char dir);
    static const char* field(const char *s, uint8_t index);
    static float fieldFloat(const char *s, uint8_t index);
    static int   fieldInt(const char *s, uint8_t index);
};

#endif // GPS_MODULE_H
