#ifndef SDCARD_MODULE_H
#define SDCARD_MODULE_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "Config.h"

// ============================================================
//  SD Card Module
//  — Initialise card
//  — Open / create log files with auto-incrementing names
//  — Append CSV rows, flush periodically
//  — Read back files
// ============================================================

class SDCardModule {
public:
    // Mount the card; returns true on success
    bool begin(uint8_t csPin = SD_CS_PIN);

    // Create a new log file (e.g. "LOG_001.csv")
    // Automatically increments to avoid overwriting
    bool openNewLog(const String &prefix = "LOG");

    // Write a header row (call once after openNewLog)
    bool writeHeader(const String &header);

    // Append one CSV line to the open log file
    bool appendLine(const String &line);

    // Force flush to card (call periodically)
    void flush();

    // Close current log file
    void closeLog();

    // Read a file's full contents into a String (small files only)
    String readFile(const String &path);

    // Utility: check remaining card space (KB)
    uint32_t freeSpaceKB();

    // Current log filename
    const String& logFilename() const;

    bool isReady() const;

private:
    File      _logFile;
    String    _logName;
    bool      _ready = false;
    SPIClass* _spi = nullptr;
};

#endif // SDCARD_MODULE_H
