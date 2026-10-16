#include "radar.h"
#include <Arduino.h>

#ifndef WOKWI_SIM
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static BLEScan* pBLEScan = nullptr;

class PortalAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        int rssi = advertisedDevice.getRSSI();
        // Close proximity detection: RSSI stronger than -68 dBm (~1.5 to 2 meters)
        if (rssi > -68) {
            radar.update(); // Hook if needed
        }
    }
};

static PortalAdvertisedDeviceCallbacks* pCallbacks = nullptr;
#endif

RadarSystem radar;

void RadarSystem::begin() {
#ifndef WOKWI_SIM
    BLEDevice::init("PortalGunC137");
    pBLEScan = BLEDevice::getScan();
    pCallbacks = new PortalAdvertisedDeviceCallbacks();
    pBLEScan->setAdvertisedDeviceCallbacks(pCallbacks);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
#endif
}

void RadarSystem::startScan() {
    scanning = true;
    alertActive = false;
    scanStartTime = millis();
    alertEndTime = 0;

#ifdef WOKWI_SIM
    nextSimAlertTime = millis() + 18000; // Trigger a simulated blip at 18s in Wokwi
#else
    lastBleScanTime = 0;
#endif
}

void RadarSystem::stopScan() {
    scanning = false;
    alertActive = false;
#ifndef WOKWI_SIM
    if (pBLEScan) {
        pBLEScan->stop();
        pBLEScan->clearResults();
    }
#endif
}

void RadarSystem::update() {
    if (!scanning) return;

    uint32_t now = millis();

    // Check if alert is expired
    if (alertActive && now >= alertEndTime) {
        alertActive = false;
    }

#ifdef WOKWI_SIM
    // In Wokwi simulation, simulate radar blips at 18s and 42s
    if (now >= nextSimAlertTime && !alertActive) {
        alertActive = true;
        alertEndTime = now + 2000;
        alertText = (nextSimAlertTime % 2 == 0) ? "FDED" : "R1CK";
        nextSimAlertTime = now + 24000; // Next simulated alert
    }
#else
    // On physical hardware: non-blocking periodic BLE scan
    if (pBLEScan && (now - lastBleScanTime > 3000)) {
        lastBleScanTime = now;
        BLEScanResults foundDevices = pBLEScan->start(2, false);
        for (int i = 0; i < foundDevices.getCount(); i++) {
            BLEAdvertisedDevice dev = foundDevices.getDevice(i);
            if (dev.getRSSI() > -68) {
                alertActive = true;
                alertEndTime = now + 2500;
                alertText = "FDED";
                break;
            }
        }
        pBLEScan->clearResults();
    }
#endif
}
