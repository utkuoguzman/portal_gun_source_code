#pragma once
#include <stdint.h>

class RadarSystem {
public:
    void begin();
    void startScan();
    void stopScan();
    void update();

    bool isScanning() const { return scanning; }
    bool isAlertActive() const { return alertActive; }
    const char* getAlertText() const { return alertText; }

private:
    bool scanning = false;
    bool alertActive = false;
    uint32_t scanStartTime = 0;
    uint32_t alertEndTime = 0;
    const char* alertText = "FDED";

#ifdef WOKWI_SIM
    uint32_t nextSimAlertTime = 0;
#else
    uint32_t lastBleScanTime = 0;
#endif
};

extern RadarSystem radar;

