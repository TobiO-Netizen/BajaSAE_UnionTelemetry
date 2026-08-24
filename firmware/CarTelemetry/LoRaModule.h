#ifndef LORA_MODULE_H
#define LORA_MODULE_H

#include <Arduino.h>
#include "Config.h"

// ============================================================
//  LoRa Module  (REYAX RYLR896 / RYLR998 AT-command radios)
//  — Configure address, network, band
//  — Send arbitrary string payloads
//  — Non-blocking receive
// ============================================================

class LoRaModule {
public:
    // Initialise UART & send AT config commands
    // Pass the HardwareSerial reference (e.g. Serial1)
    bool begin(HardwareSerial &serial,
               long baud       = LORA_BAUD,
               int  rxPin      = LORA_RXD,
               int  txPin      = LORA_TXD,
               int  address    = LORA_ADDRESS,
               int  networkId  = LORA_NETWORK_ID,
               long band       = LORA_BAND);

    // Transmit a string to target address
    bool send(const String &message, int targetAddr = LORA_TARGET_ADDR);

    // Check for incoming data (call in loop); returns true if a
    // complete message was received — retrieve it with lastMessage()
    bool receive();

    // Last received payload
    const String& lastMessage() const;

    // Send a raw AT command and return the response line
    String sendAT(const String &cmd, unsigned long timeoutMs = 500);

private:
    HardwareSerial *_serial = nullptr;
    String _lastMsg;
};

#endif // LORA_MODULE_H
