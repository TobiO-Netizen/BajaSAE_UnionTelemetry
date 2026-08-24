#include "OBD2Module.h"

// ── Public API ───────────────────────────────────────────────
bool OBD2Module::begin(gpio_num_t txPin, gpio_num_t rxPin) {
    _mutex = xSemaphoreCreateMutex();
    if (!_mutex) return false;

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL);
    g.rx_queue_len = 32;
    g.tx_queue_len = 8;

    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g, &t, &f) != ESP_OK) return false;
    if (twai_start() != ESP_OK) {
        twai_driver_uninstall();
        return false;
    }

    _driverInstalled = true;
    memset((void*)&_cache, 0, sizeof(_cache));
    return true;
}

void OBD2Module::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        _polling = false;
    }
    if (_driverInstalled) {
        twai_stop();
        twai_driver_uninstall();
        _driverInstalled = false;
    }
}

bool OBD2Module::isConnected() {
    if (!_driverInstalled) return false;

    // PID 0x00 — every OBD2 ECU must respond to this
    twai_message_t msg = {};
    msg.identifier = 0x7DF;
    msg.data_length_code = 8;
    msg.data[0] = 0x02;
    msg.data[1] = 0x01;
    msg.data[2] = 0x00;
    for (int i = 3; i < 8; i++) msg.data[i] = 0xAA;

    // Flush stale messages
    twai_message_t flush;
    while (twai_receive(&flush, pdMS_TO_TICKS(1)) == ESP_OK) {}

    if (twai_transmit(&msg, pdMS_TO_TICKS(100)) != ESP_OK) {
        return false;
    }

    // Wait for ECU response (0x7E8–0x7EF)
    uint32_t start = millis();
    while (millis() - start < 200) {
        twai_message_t resp;
        if (twai_receive(&resp, pdMS_TO_TICKS(20)) == ESP_OK) {
            if (resp.identifier >= 0x7E8 && resp.identifier <= 0x7EF
                && resp.data[1] == 0x41 && resp.data[2] == 0x00) {
                return true;
            }
        }
    }
    return false;
}

void OBD2Module::startPolling() {
    if (_polling) return;

    xTaskCreatePinnedToCore(
        obdTaskWrapper,
        "obd_poll",
        4096,
        this,
        1,
        &_taskHandle,
        0              // core 0
    );
    _polling = true;
}

// ── Data access (thread-safe) ────────────────────────────────
OBDData OBD2Module::getSnapshot() {
    OBDData snap;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    snap = const_cast<OBDData&>(_cache);
    xSemaphoreGive(_mutex);
    return snap;
}

float OBD2Module::getRPM()       { return getSnapshot().rpm; }
float OBD2Module::getSpeedMph()  { return getSnapshot().speedMph; }
float OBD2Module::getSpeedKph()  { return getSnapshot().speedKph; }
float OBD2Module::getCoolantC()  { return getSnapshot().coolantC; }
float OBD2Module::getCoolantF()  { return getSnapshot().coolantF; }
float OBD2Module::getThrottle()  { return getSnapshot().throttle; }

bool OBD2Module::isFresh(uint32_t timestamp, uint32_t maxAgeMs) {
    return (millis() - timestamp) < maxAgeMs;
}

bool OBD2Module::isPolling() const { return _polling; }

// ── CAN helpers ──────────────────────────────────────────────
bool OBD2Module::requestPID(uint8_t pid) {
    twai_message_t msg = {};
    msg.identifier = 0x7DF;
    msg.data_length_code = 8;
    msg.data[0] = 0x02;
    msg.data[1] = 0x01;
    msg.data[2] = pid;
    for (int i = 3; i < 8; i++) msg.data[i] = 0xAA;
    return twai_transmit(&msg, pdMS_TO_TICKS(50)) == ESP_OK;
}

bool OBD2Module::readResponse(uint8_t expectedPid, uint8_t *out,
                               int *outLen, uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            if (msg.identifier >= 0x7E8 && msg.identifier <= 0x7EF
                && msg.data[1] == 0x41 && msg.data[2] == expectedPid) {
                *outLen = msg.data[0] - 2;
                memcpy(out, &msg.data[3], *outLen);
                return true;
            }
        }
    }
    return false;
}

void OBD2Module::updateCache(uint8_t pid, const uint8_t *d) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    uint32_t now = millis();
    switch (pid) {
        case PID_RPM:
            _cache.rpm = ((d[0] << 8) | d[1]) / 4.0f;
            _cache.rpm_ts = now;
            break;
        case PID_SPEED:
            _cache.speedKph = d[0];
            _cache.speedMph = d[0] * 0.621371f;
            _cache.speed_ts = now;
            break;
        case PID_COOLANT:
            _cache.coolantC = d[0] - 40;
            _cache.coolantF = _cache.coolantC * 9.0f / 5.0f + 32.0f;
            _cache.coolant_ts = now;
            break;
        case PID_THROTTLE:
            _cache.throttle = d[0] * 100.0f / 255.0f;
            _cache.throttle_ts = now;
            break;
    }
    xSemaphoreGive(_mutex);
}

void OBD2Module::pollOne(uint8_t pid) {
    uint8_t buf[6];
    int len;
    if (requestPID(pid) && readResponse(pid, buf, &len)) {
        updateCache(pid, buf);
    }
}

// ── Background task ──────────────────────────────────────────
void OBD2Module::obdTaskWrapper(void *param) {
    OBD2Module *self = static_cast<OBD2Module*>(param);
    self->obdTaskLoop();
}

void OBD2Module::obdTaskLoop() {
    // RPM and speed every cycle, coolant every 5th
    uint8_t fast[] = {PID_RPM, PID_SPEED, PID_THROTTLE};
    uint32_t iter = 0;

    while (true) {
        for (uint8_t p : fast) pollOne(p);
        if (iter % 5 == 0) {
            pollOne(PID_COOLANT);
        }
        iter++;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
