#include "LoRaModule.h"

bool LoRaModule::begin(HardwareSerial &serial, long baud, int rxPin,
                       int txPin, int address, int networkId, long band) {
    _serial = &serial;
    _serial->begin(baud, SERIAL_8N1, rxPin, txPin);
    delay(200);

    // Configure module
    sendAT("AT+ADDRESS=" + String(address));
    sendAT("AT+NETWORKID=" + String(networkId));
    sendAT("AT+BAND=" + String(band));

    // Quick self-check
    String resp = sendAT("AT");
    return (resp.indexOf("+OK") >= 0 || resp.indexOf("OK") >= 0);
}

bool LoRaModule::send(const String &message, int targetAddr) {
    if (!_serial) return false;

    String cmd = "AT+SEND=" + String(targetAddr) + "," +
                 String(message.length()) + "," + message;
    String resp = sendAT(cmd, 1000);
    return (resp.indexOf("+OK") >= 0);
}

bool LoRaModule::receive() {
    if (!_serial || !_serial->available()) return false;

    String line = _serial->readStringUntil('\n');
    line.trim();

    // Incoming format: +RCV=<addr>,<len>,<data>,<RSSI>,<SNR>
    if (line.startsWith("+RCV=")) {
        int firstComma  = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        int thirdComma  = line.indexOf(',', secondComma + 1);
        if (secondComma > 0 && thirdComma > 0) {
            _lastMsg = line.substring(secondComma + 1, thirdComma);
            return true;
        }
    }
    return false;
}

const String& LoRaModule::lastMessage() const { return _lastMsg; }

String LoRaModule::sendAT(const String &cmd, unsigned long timeoutMs) {
    // Drain anything stale
    while (_serial->available()) _serial->read();

    _serial->println(cmd);
    unsigned long start = millis();
    String response;
    while (millis() - start < timeoutMs) {
        while (_serial->available()) {
            char c = _serial->read();
            response += c;
        }
        if (response.indexOf("+OK") >= 0 || response.indexOf("+ERR") >= 0)
            break;
    }
    response.trim();
    return response;
}
