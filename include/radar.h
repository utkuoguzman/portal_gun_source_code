#pragma once
#include <stdint.h>

class RadarSystem {
public:
    void begin();
    void startScan();
    void stopScan();
    void update();

    bool isScanning() const { return scanning; }
    bool isPaired() const { return phonePaired; }
    float getProximity() const { return proximity; } // 0.0f (far / green) -> 1.0f (very close / red)
    int getRssi() const { return (int)smoothedRssi; }

    // Legacy compatibility helpers
    bool isAlertActive() const { return phonePaired; }
    const char* getAlertText() const { return "R1CK"; }

private:
    bool scanning = false;
    bool phonePaired = false;
    float proximity = 0.0f;
    float smoothedRssi = -88.0f;
    uint32_t scanStartTime = 0;

#ifdef WOKWI_SIM
    uint32_t nextSimAlertTime = 0;
#else
    uint32_t lastBleScanTime = 0;
#endif
};

extern RadarSystem radar;
