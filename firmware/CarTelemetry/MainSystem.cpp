#include "MainSystem.h"
#include <Wire.h>
#include <lvgl.h>

// SquareLine Studio screens
extern lv_obj_t *ui_ScreenBaja;
extern lv_obj_t *ui_ScreenStandard;
extern lv_obj_t *ui_PointerBaja;
extern lv_obj_t *ui_PointerStandard;

// ui_Baja widgets
extern lv_obj_t *ui_SpeedGPS;
extern lv_obj_t *ui_AcccelerationXYZBajaData;
extern lv_obj_t *ui_TempData;
extern lv_obj_t *ui_TempBar;

// ui_Standard widgets
extern lv_obj_t *ui_Speed2;
extern lv_obj_t *ui_AcccelerationXYZData;
extern lv_obj_t *ui_ThrottlePosData;
extern lv_obj_t *ui_RPMData;
extern lv_obj_t *ui_EngineCoolantTempData;

// Needle rotation — maps value to 270 degree gauge sweep
static void updateSpeedNeedle(lv_obj_t *needle, float speedMph, float maxMph) {
    float ratio = speedMph / maxMph;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    int16_t angle = -1250 + (int16_t)(ratio * 2700.0f);
    lv_img_set_angle(needle, angle);
}

// -- CSV header matching buildCSVRow() --
static const char *CSV_HEADER =
    "millis,packetNum,"
    "accelX_g,accelY_g,accelZ_g,"
    "gyroX,gyroY,gyroZ,"
    "objTempF,ambTempF,"
    "forceN,forceLbf,"
    "lat,lon,alt_m,speedMph,satellites,"
    "rpm,obdSpeedMph,coolantF,throttle,"
    "timestamp";

// ---------------------------------------------------------------
bool MainSystem::begin() {
    Serial.begin(DEBUG_BAUD);
    delay(1000);

    // Deselect all SPI devices immediately — prevents bus collisions
    pinMode(SCREEN_CS_PIN, OUTPUT);
    digitalWrite(SCREEN_CS_PIN, HIGH);
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    Wire.begin(MPU_SDA, MPU_SCL); 

    Serial.println(F("\n============================"));
    Serial.println(F("  Car Telemetry System"));
    Serial.println(F("============================\n"));

    bool allOk = true;

    // -- MPU6050 --
    Serial.print(F("[MPU6050]  Initialising... "));
    if (mpu.begin()) {
        Serial.println(F("OK"));
        Serial.println(F("           Calibrating — keep vehicle still..."));
        mpu.calibrate(300);
        Serial.println(F("           Calibration done."));
    } else {
        Serial.println(F("FAIL"));
        allOk = false;
    }

    // -- Temp sensor --
    Serial.print(F("[MLX90614] Initialising... "));
    if (temp.begin()) {
        Serial.println(F("OK"));
    } else {
        Serial.println(F("FAIL"));
        allOk = false;
    }

    // -- Strain gauge / HX711 --
    Serial.print(F("[HX711]    Initialising... "));
    if (strain.begin(HX711_DOUT_PIN, HX711_SCK_PIN, HX711_GAIN)) {
        Serial.println(F("OK"));
        Serial.println(F("           Taring — remove all load..."));
        strain.tare(20);
        strain.setCalibrationFactor(HX711_CAL_FACTOR);
        Serial.println(F("           Tare done."));
    } else {
        Serial.println(F("FAIL"));
        allOk = false;
    }

    // -- GPS --
    Serial.print(F("[GPS]      Initialising... "));
    if (gps.begin(Serial2)) {
        Serial.println(F("OK  (waiting for fix)"));
    } else {
        Serial.println(F("FAIL"));
        allOk = false;
    }

    // -- SD Card --
    if (!sd.isReady()) {
        Serial.print(F("[SD Card]  Mounting... "));
        if (sd.begin()) {
            Serial.println(F("OK"));
        } else {
            Serial.println(F("FAIL — logging disabled"));
        }
    } else {
        Serial.println(F("[SD Card]  Already mounted"));
    }
    if (sd.isReady()) {
        sd.openNewLog("TEL");
        sd.writeHeader(CSV_HEADER);
        Serial.print(F("           Log file: "));
        Serial.println(sd.logFilename());
    }

    // -- LoRa --
    Serial.print(F("[LoRa]     Initialising... "));
    if (lora.begin(Serial1)) {
        Serial.println(F("OK"));
    } else {
        Serial.println(F("FAIL"));
        allOk = false;
    }

    // -- OBD2 / CAN Bus --
    Serial.print(F("[OBD2]     Initialising CAN bus... "));
    if (obd.begin(CAN_TX_PIN, CAN_RX_PIN)) {
        Serial.println(F("OK"));
        Serial.print(F("           Detecting ECU... "));
        if (obd.isConnected()) {
            Serial.println(F("CONNECTED"));
            _obdConnected = true;
            obd.startPolling();
            Serial.println(F("           Background polling started on core 0"));
        } else {
            Serial.println(F("NOT DETECTED (car off or not plugged in)"));
            _obdConnected = false;
        }
    } else {
        Serial.println(F("FAIL"));
        _obdConnected = false;
    }

    Serial.println();
    Serial.println(allOk ? F("All systems GO.")
                         : F("Some modules failed — check wiring."));
    if (_obdConnected) {
        Serial.println(F("OBD2 active — logging engine data."));
        lv_scr_load(ui_ScreenStandard);
    } else {
        Serial.println(F("OBD2 inactive — sensor-only mode."));
        lv_scr_load(ui_ScreenBaja);
    }

    // Set needle pivot points — adjust x,y to match your image dimensions
    //lv_img_set_pivot(ui_PointerBaja, 15, 120);      // TODO: adjust to your image
    //lv_img_set_pivot(ui_PointerStandard, 15, 120);   // TODO: adjust to your image

    // Init temp bar range
    lv_bar_set_range(ui_TempBar, -40, 500);   // -40F to 300F
    lv_bar_set_value(ui_TempBar, 0, LV_ANIM_ON);

    Serial.println();

    unsigned long now = millis();
    _lastSensorMs = now;
    _lastLogMs    = now;
    _lastLoraMs   = now;

    return allOk;
}

// ---------------------------------------------------------------
void MainSystem::update() {
    unsigned long now = millis();

    // -- Sensor reads --
    if (now - _lastSensorMs >= MAIN_LOOP_INTERVAL_MS) {
        _lastSensorMs = now;
        readAllSensors();
    }

    // -- SD logging --
    if (now - _lastLogMs >= SD_LOG_INTERVAL_MS) {
        _lastLogMs = now;
        logToSD();
    }

    // -- LoRa transmit --
    if (now - _lastLoraMs >= LORA_SEND_INTERVAL_MS) {
        _lastLoraMs = now;
        transmitLoRa();
    }

    // Always feed GPS bytes
    gps.update();

    // -- OBD2 hot-plug detection (every 5 seconds) --
    static unsigned long lastObdCheck = 0;
    if (now - lastObdCheck >= 5000) {
        lastObdCheck = now;
        bool wasConnected = _obdConnected;
        _obdConnected = obd.isConnected();

        if (_obdConnected && !wasConnected) {
            obd.startPolling();
            lv_scr_load_anim(ui_ScreenStandard, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
            Serial.println(F("[OBD2]   ECU detected — switching to OBD screen"));
        } else if (!_obdConnected && wasConnected) {
            obd.stop();
            lv_scr_load_anim(ui_ScreenBaja, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
            Serial.println(F("[OBD2]   ECU lost — switching to sensor screen"));
        }
    }

    // Check for incoming LoRa commands
    if (lora.receive()) {
        Serial.print(F("[LoRa RX] "));
        Serial.println(lora.lastMessage());
    }
}

// ---------------------------------------------------------------
void MainSystem::readAllSensors() {
    mpu.update();
    temp.update();
    strain.update();

    const MPU6050Reading &m = mpu.getReading();
    float objTempF = temp.getObjectTempF();

    Serial.printf("[Accel]  X=%.2f Y=%.2f Z=%.2f g\n",
                  m.accel.x, m.accel.y, m.accel.z);
    Serial.printf("[Gyro]   X=%.1f Y=%.1f Z=%.1f deg/s\n",
                  m.gyro.x, m.gyro.y, m.gyro.z);
    Serial.printf("[Temp]   Obj=%.1fF  Amb=%.1fF\n",
                  objTempF, temp.getAmbientTempF());
    Serial.printf("[Strain] %.2f N  (%.2f lbf)\n",
                  strain.getForceN(), strain.getForceLbf());

    if (gps.hasFix()) {
        const GPSData &g = gps.getData();
        Serial.printf("[GPS]    %.6f, %.6f  alt=%.1fm  speed=%.1fmph  sats=%d  %s\n",
                      g.latitude, g.longitude, g.altitude,
                      g.speedMph, g.satellites, gps.timestamp().c_str());
    } else {
        Serial.println(F("[GPS]    No fix yet"));
    }

    if (_obdConnected) {
        OBDData o = obd.getSnapshot();
        Serial.printf("[OBD2]   RPM=%.0f  Spd=%.0fmph  Coolant=%.0fF  Thr=%.1f%%\n",
                      o.rpm, o.speedMph, o.coolantF, o.throttle);
    }

    // ── Update UI ────────────────────────────────────────────
    char buf[32];
    float speed = gps.hasFix() ? gps.getData().speedMph : 0.0f;

    // -- Needles (both screens) --
    updateSpeedNeedle(ui_PointerBaja, speed, 160.0f);
    updateSpeedNeedle(ui_PointerStandard, speed, 160.0f);

    // ── ui_Baja screen ──────────────────────────────────────

    // Speed label
    snprintf(buf, sizeof(buf), "%.0f", speed);
    lv_label_set_text(ui_SpeedGPS, buf);

    // Acceleration  "X | Y | Z"  (2 decimal places)
    snprintf(buf, sizeof(buf), "%.2f | %.2f | %.2f",
             m.accel.x, m.accel.y, m.accel.z);
    lv_label_set_text(ui_AcccelerationXYZBajaData, buf);

    // Temperature label
    snprintf(buf, sizeof(buf), "%.1f F", objTempF);
    lv_label_set_text(ui_TempData, buf);

    // Temperature bar
    lv_bar_set_value(ui_TempBar, (int32_t)objTempF, LV_ANIM_ON);

    // ── ui_Standard screen (OBD2 data) ──────────────────────

    if (_obdConnected) {
        OBDData o = obd.getSnapshot();

        // Speed label
        snprintf(buf, sizeof(buf), "%.0f", o.speedMph);
        lv_label_set_text(ui_Speed2, buf);

        // Acceleration  "X | Y | Z"
        snprintf(buf, sizeof(buf), "%.2f | %.2f | %.2f",
                 m.accel.x, m.accel.y, m.accel.z);
        lv_label_set_text(ui_AcccelerationXYZData, buf);

        // Throttle position
        snprintf(buf, sizeof(buf), "%.1f%%", o.throttle);
        lv_label_set_text(ui_ThrottlePosData, buf);

        // RPM
        snprintf(buf, sizeof(buf), "%.0f", o.rpm);
        lv_label_set_text(ui_RPMData, buf);

        // Engine coolant temp
        snprintf(buf, sizeof(buf), "%.0f F", o.coolantF);
        lv_label_set_text(ui_EngineCoolantTempData, buf);
    } else {
        // Show dashes when OBD not connected
        lv_label_set_text(ui_Speed2, "--");
        lv_label_set_text(ui_AcccelerationXYZData, "-- | -- | --");
        lv_label_set_text(ui_ThrottlePosData, "--%");
        lv_label_set_text(ui_RPMData, "--");
        lv_label_set_text(ui_EngineCoolantTempData, "-- F");
    }
}

// ---------------------------------------------------------------
String MainSystem::buildCSVRow() {
    const MPU6050Reading &m = mpu.getReading();
    const GPSData        &g = gps.getData();

    char row[320];

    if (_obdConnected) {
        OBDData o = obd.getSnapshot();
        snprintf(row, sizeof(row),
            "%lu,%lu,"
            "%.3f,%.3f,%.3f,"
            "%.2f,%.2f,%.2f,"
            "%.1f,%.1f,"
            "%.2f,%.2f,"
            "%.6f,%.6f,%.1f,%.1f,%d,"
            "%.0f,%.1f,%.1f,%.1f,"
            "%s",
            millis(), _packetCount,
            m.accel.x, m.accel.y, m.accel.z,
            m.gyro.x,  m.gyro.y,  m.gyro.z,
            temp.getObjectTempF(), temp.getAmbientTempF(),
            strain.getForceN(), strain.getForceLbf(),
            g.latitude, g.longitude, g.altitude,
            g.speedMph, g.satellites,
            o.rpm, o.speedMph, o.coolantF, o.throttle,
            gps.timestamp().c_str()
        );
    } else {
        snprintf(row, sizeof(row),
            "%lu,%lu,"
            "%.3f,%.3f,%.3f,"
            "%.2f,%.2f,%.2f,"
            "%.1f,%.1f,"
            "%.2f,%.2f,"
            "%.6f,%.6f,%.1f,%.1f,%d,"
            ",,,,"
            "%s",
            millis(), _packetCount,
            m.accel.x, m.accel.y, m.accel.z,
            m.gyro.x,  m.gyro.y,  m.gyro.z,
            temp.getObjectTempF(), temp.getAmbientTempF(),
            strain.getForceN(), strain.getForceLbf(),
            g.latitude, g.longitude, g.altitude,
            g.speedMph, g.satellites,
            gps.timestamp().c_str()
        );
    }
    return String(row);
}

String MainSystem::buildLoRaPayload() {
    // pkt|aX|aY|aZ|gX|gY|gZ|spdMph|objF|forceN|lat|lon|sats
    const MPU6050Reading &m = mpu.getReading();
    const GPSData        &g = gps.getData();

    char buf[160];
    snprintf(buf, sizeof(buf),
        "%lu|%.2f|%.2f|%.2f|%.1f|%.1f|%.1f|%.1f|%.1f|%.2f|%.6f|%.6f|%d",
        _packetCount,
        m.accel.x, m.accel.y, m.accel.z,
        m.gyro.x,  m.gyro.y,  m.gyro.z,
        g.speedMph,
        temp.getObjectTempF(),
        strain.getForceN(),
        g.latitude, g.longitude,
        g.satellites
    );
    return String(buf);
}

void MainSystem::logToSD() {
    if (!sd.isReady()) return;
    sd.appendLine(buildCSVRow());
    if (_packetCount % 10 == 0) sd.flush();
}

void MainSystem::transmitLoRa() {
    _packetCount++;
    String payload = buildLoRaPayload();

    Serial.print(F("[LoRa TX] "));
    Serial.println(payload);

    lora.send(payload);
}