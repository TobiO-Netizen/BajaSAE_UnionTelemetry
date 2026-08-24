#include "SDCardModule.h"

bool SDCardModule::begin(uint8_t csPin) {
    _spi = new SPIClass(HSPI);   // created here, not at global scope
    // Skip _spi->begin() — HSPI already configured by TFT_eSPI
    if (!SD.begin(csPin, *_spi, 4000000)) {
        _ready = false;
        return false;
    }
    _ready = true;
    return true;
}
bool SDCardModule::openNewLog(const String &prefix) {
    if (!_ready) return false;

    // Find next available index
    for (int i = 1; i <= 999; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "/%s_%03d.csv", prefix.c_str(), i);
        if (!SD.exists(buf)) {
            _logFile = SD.open(buf, FILE_WRITE);
            if (_logFile) {
                _logName = String(buf);
                return true;
            }
            return false;
        }
    }
    return false;   // 999 logs — impressive
}

bool SDCardModule::writeHeader(const String &header) {
    if (!_logFile) return false;
    _logFile.println(header);
    _logFile.flush();
    return true;
}

bool SDCardModule::appendLine(const String &line) {
    if (!_logFile) return false;
    _logFile.println(line);
    return true;
}

void SDCardModule::flush() {
    if (_logFile) _logFile.flush();
}

void SDCardModule::closeLog() {
    if (_logFile) {
        _logFile.flush();
        _logFile.close();
    }
}

String SDCardModule::readFile(const String &path) {
    String content;
    File f = SD.open(path, FILE_READ);
    if (!f) return content;
    while (f.available()) {
        content += (char)f.read();
    }
    f.close();
    return content;
}

uint32_t SDCardModule::freeSpaceKB() {
    // SD library doesn't expose free space on all platforms;
    // return total size as a rough upper-bound indicator.
    return (uint32_t)(SD.totalBytes() / 1024ULL);
}

const String& SDCardModule::logFilename() const { return _logName; }
bool SDCardModule::isReady() const { return _ready; }
