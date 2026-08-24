#include "GPSModule.h"
#include <string.h>
#include <stdlib.h>

// ── Public ───────────────────────────────────────────────────
bool GPSModule::begin(HardwareSerial &serial, long baud, int rxPin, int txPin) {
    _serial = &serial;
    _serial->begin(baud, SERIAL_8N1, rxPin, txPin);
    memset(&_data, 0, sizeof(_data));
    return true;
}

void GPSModule::update() {
    if (!_serial) return;
    while (_serial->available()) {
        char c = _serial->read();
        if (c == '\n' || c == '\r') {
            if (_bufIdx > 5) {
                _buf[_bufIdx] = '\0';
                parseSentence(_buf);
            }
            _bufIdx = 0;
        } else if (_bufIdx < sizeof(_buf) - 1) {
            _buf[_bufIdx++] = c;
        }
    }
}

const GPSData& GPSModule::getData()  const { return _data; }
bool  GPSModule::hasFix()            const { return _data.hasFix; }
float GPSModule::latitude()          const { return _data.latitude; }
float GPSModule::longitude()         const { return _data.longitude; }

String GPSModule::timestamp() const {
    char ts[24];
    snprintf(ts, sizeof(ts), "%04u-%02u-%02u %02u:%02u:%02u",
             _data.year, _data.month, _data.day,
             _data.hour, _data.minute, _data.second);
    return String(ts);
}

// ── Sentence router ──────────────────────────────────────────
void GPSModule::parseSentence(const char *s) {
    if (strstr(s, "GGA")) parseGGA(s);
    if (strstr(s, "RMC")) parseRMC(s);
}

// ── $GPGGA — fix quality, satellites, altitude ───────────────
//  0      1        2       3 4        5 6 7  8   9   10 ...
// $GPGGA,hhmmss.ss,lat,    N,lon,     E,q,sv,hdop,alt,M,...
void GPSModule::parseGGA(const char *s) {
    int quality = fieldInt(s, 6);
    _data.hasFix     = (quality >= 1);
    _data.satellites = (uint8_t)fieldInt(s, 7);

    if (_data.hasFix) {
        char latDir = 'N', lonDir = 'E';
        const char *f3 = field(s, 3); if (f3 && *f3) latDir = *f3;
        const char *f5 = field(s, 5); if (f5 && *f5) lonDir = *f5;

        char latRaw[16] = {0}, lonRaw[16] = {0};
        const char *f2 = field(s, 2);
        const char *f4 = field(s, 4);
        if (f2) strncpy(latRaw, f2, 15);
        if (f4) strncpy(lonRaw, f4, 15);

        // Terminate at comma
        for (int i = 0; i < 15; i++) { if (latRaw[i] == ',') latRaw[i] = 0; }
        for (int i = 0; i < 15; i++) { if (lonRaw[i] == ',') lonRaw[i] = 0; }

        _data.latitude  = nmeaToDecimal(latRaw, latDir);
        _data.longitude = nmeaToDecimal(lonRaw, lonDir);
        _data.altitude  = fieldFloat(s, 9);
    }
}

// ── $GPRMC — speed, course, date/time ────────────────────────
//  0      1        2 3       4 5        6 7    8    9      10
// $GPRMC,hhmmss.ss,A,lat,    N,lon,     E,knots,crs,ddmmyy,mv...
void GPSModule::parseRMC(const char *s) {
    // Time  (field 1)
    float rawTime = fieldFloat(s, 1);
    int iTime = (int)rawTime;
    _data.hour   = iTime / 10000;
    _data.minute = (iTime / 100) % 100;
    _data.second = iTime % 100;

    // Date  (field 9)
    int rawDate = fieldInt(s, 9);
    _data.day   = rawDate / 10000;
    _data.month = (rawDate / 100) % 100;
    _data.year  = 2000 + (rawDate % 100);

    // Speed & course
    _data.speedKnots = fieldFloat(s, 7);
    _data.speedKph   = _data.speedKnots * 1.852f;
    _data.speedMph   = _data.speedKnots * 1.15078f;
    _data.course     = fieldFloat(s, 8);
}

// ── NMEA helpers ─────────────────────────────────────────────
float GPSModule::nmeaToDecimal(const char *raw, char dir) {
    if (!raw || raw[0] == '\0') return 0;
    // Format: DDDMM.MMMM
    float val = atof(raw);
    int   deg = (int)(val / 100);
    float min = val - deg * 100.0f;
    float dec = deg + min / 60.0f;
    if (dir == 'S' || dir == 'W') dec = -dec;
    return dec;
}

const char* GPSModule::field(const char *s, uint8_t index) {
    uint8_t commas = 0;
    while (*s) {
        if (*s == ',') {
            commas++;
            if (commas == index) return s + 1;
        }
        s++;
    }
    return nullptr;
}

float GPSModule::fieldFloat(const char *s, uint8_t index) {
    const char *f = field(s, index);
    return f ? atof(f) : 0.0f;
}

int GPSModule::fieldInt(const char *s, uint8_t index) {
    const char *f = field(s, index);
    return f ? atoi(f) : 0;
}
