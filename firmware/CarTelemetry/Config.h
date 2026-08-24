#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
//  PIN ASSIGNMENTS — edit these to match your wiring
// ============================================================

// LoRa (REYAX RYLR896 / similar) — UART
#define LORA_RXD        3
#define LORA_TXD        4
#define LORA_BAUD       115200
#define LORA_ADDRESS    1
#define LORA_NETWORK_ID 5
#define LORA_BAND       915000000
#define LORA_TARGET_ADDR 2        // destination address for sends

// GPS (NEO-6M) — UART
#define GPS_RXD         2
#define GPS_TXD         1
#define GPS_BAUD        9600

// SD Card — SPI (shares MOSI/MISO/SCK with ILI9341 screen)
#define SD_CS_PIN       21        // chip-select for SD
#define SCREEN_CS_PIN   5         // chip-select for ILI9341 — must be HIGH when SD talks

// MPU6050 — I2C  (uses default SDA/SCL on most boards)
 #define MPU_SDA      12
 #define MPU_SCL      11

// GY-906 MLX90614 — I2C (shares bus with MPU6050)
// Default I2C address 0x5A

// Strain Gauge (350Ω) + HX711 — digital GPIO
#define HX711_DOUT_PIN  10         // data out
#define HX711_SCK_PIN   9         // clock
#define HX711_GAIN      128       // 128 or 64 (ch-A), 32 (ch-B)
#define HX711_CAL_FACTOR 420.0f   // counts per Newton — calibrate with known weight

// OBD2 CAN Bus — TWAI (via CAN transceiver)
#define CAN_TX_PIN      GPIO_NUM_13
#define CAN_RX_PIN      GPIO_NUM_14

// ============================================================
//  TIMING (milliseconds)
// ============================================================
#define MAIN_LOOP_INTERVAL_MS   500   // telemetry tick rate
#define GPS_READ_INTERVAL_MS    1000
#define SD_LOG_INTERVAL_MS      1000
#define LORA_SEND_INTERVAL_MS   2000

// ============================================================
//  SERIAL DEBUG
// ============================================================
#define DEBUG_BAUD 115200

#endif // CONFIG_H
