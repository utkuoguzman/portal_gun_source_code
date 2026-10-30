#include "radar.h"
#include <Arduino.h>
#include <string.h>

#ifndef WOKWI_SIM
#include "esp_bt.h"

static volatile bool vPhonePaired = false;
static volatile int8_t windowPeakRssi = -100;
static volatile uint32_t lastPacketMs = 0;

// Queued pairing replies (populated in VHCI callback, transmitted safely in update())
static uint8_t peerBdAddr[6] = {0};
static uint8_t savedPeerMac[6] = {0};
static uint8_t savedLinkKey[16] = {0};
static volatile bool hasSavedLinkKey = false;

static volatile bool pendingAcceptConn = false;
static volatile bool pendingIoCapReply = false;
static volatile bool pendingUserConfirmReply = false;
static volatile bool pendingLinkKeyReply = false;
static volatile bool pendingLinkKeyNegReply = false;
static volatile bool pendingPinCodeReply = false;
static volatile uint16_t activeAclHandle = 0xFFFF;
static volatile uint32_t pairingDoneMs = 0;

static volatile bool cmdCompleteReceived = false;

static void controller_rcv_pkt_ready(void) {}

static int host_rcv_pkt(uint8_t *data, uint16_t len) {
    if (len < 3 || data[0] != 0x04) return 0; // Must be HCI Event packet (0x04)
    uint8_t eventCode = data[1];

    // Command Complete (0x0E) or Command Status (0x0F)
    if (eventCode == 0x0E || eventCode == 0x0F) {
        cmdCompleteReceived = true;
        // Check if this is Command Complete for HCI_Read_RSSI (Opcode 0x1405)
        if (eventCode == 0x0E && len >= 10 && data[4] == 0x05 && data[5] == 0x14 && data[6] == 0x00) {
            int8_t rssi = (int8_t)data[9];
            // If Classic BT reports raw dBm (< 0), use it directly
            if (rssi < 0 && rssi > -92) {
                if (rssi > windowPeakRssi) windowPeakRssi = rssi;
                lastPacketMs = millis();
            }
        }
        return 0;
    }

    // 1. BLE Advertising Report: Event 0x3E (LE Meta), Subevent 0x02
    if (eventCode == 0x3E && len >= 14 && data[3] == 0x02) {
        uint8_t numReports = data[4];
        int offset = 5;
        for (uint8_t i = 0; i < numReports && offset + 9 < len; i++) {
            uint8_t dataLen = data[offset + 8];
            int rssiPos = offset + 9 + dataLen;
            if (rssiPos < len) {
                int8_t rssi = (int8_t)data[rssiPos];
                if (rssi > -92 && rssi < 0) {
                    if (rssi > windowPeakRssi) windowPeakRssi = rssi;
                    lastPacketMs = millis();
                }
            }
            offset = rssiPos + 1;
        }
    }
    // 2. BLE Connection Complete: Event 0x3E (LE Meta), Subevent 0x01
    else if (eventCode == 0x3E && len >= 10 && data[3] == 0x01) {
        vPhonePaired = true;
        pairingDoneMs = millis();
        lastPacketMs = millis();
    }
    // 3. Classic BT Connection Request (Event 0x04) -> Phone tapped "Portal-Gun-C137"!
    else if (eventCode == 0x04 && len >= 9) {
        memcpy(peerBdAddr, &data[3], 6);
        pendingAcceptConn = true;
        lastPacketMs = millis();
    }
    // 4. Classic BT Connection Complete (Event 0x03)
    else if (eventCode == 0x03 && len >= 11) {
        if (data[3] == 0x00) {
            activeAclHandle = (uint16_t)data[4] | ((uint16_t)(data[5] & 0x0F) << 8);
            // If reconnecting with an already-paired phone, mark paired
            if (hasSavedLinkKey && memcmp(&data[6], savedPeerMac, 6) == 0) {
                vPhonePaired = true;
                pairingDoneMs = millis();
            }
        }
    }
    // 5. Classic BT Disconnection Complete (Event 0x05)
    else if (eventCode == 0x05) {
        activeAclHandle = 0xFFFF;
    }
    // 6. Classic BT Link Key Request (Event 0x18)
    else if (eventCode == 0x18 && len >= 9) {
        memcpy(peerBdAddr, &data[3], 6);
        if (hasSavedLinkKey && memcmp(peerBdAddr, savedPeerMac, 6) == 0) {
            pendingLinkKeyReply = true;
        } else {
            pendingLinkKeyNegReply = true;
        }
    }
    // 7. Classic BT Link Key Notification (Event 0x1B) -> Pairing 100% succeeded & bonded!
    else if (eventCode == 0x1B && len >= 25) {
        memcpy(savedPeerMac, &data[3], 6);
        memcpy(savedLinkKey, &data[9], 16);
        hasSavedLinkKey = true;
        vPhonePaired = true;
        pairingDoneMs = millis();
        if (-50 > windowPeakRssi) windowPeakRssi = -50;
        lastPacketMs = millis();
    }
    // 8. Classic BT Simple Pairing Complete (Event 0x36)
    else if (eventCode == 0x36 && len >= 10) {
        if (data[3] == 0x00) {
            vPhonePaired = true;
            pairingDoneMs = millis();
            if (-50 > windowPeakRssi) windowPeakRssi = -50;
            lastPacketMs = millis();
        }
    }
    // 9. Classic BT Authentication Complete (Event 0x06)
    else if (eventCode == 0x06 && len >= 6) {
        if (data[3] == 0x00) {
            vPhonePaired = true;
            pairingDoneMs = millis();
        }
    }
    // 10. Classic BT PIN Code Request (Event 0x16) - fallback for legacy BT
    else if (eventCode == 0x16 && len >= 9) {
        memcpy(peerBdAddr, &data[3], 6);
        pendingPinCodeReply = true;
    }
    // 11. Classic BT SSP IO Capability Request (Event 0x31)
    else if (eventCode == 0x31 && len >= 9) {
        memcpy(peerBdAddr, &data[3], 6);
        pendingIoCapReply = true;
    }
    // 12. Classic BT SSP User Confirmation Request (Event 0x33) -> Auto-accept "Just Works" (no password!)
    else if (eventCode == 0x33 && len >= 9) {
        memcpy(peerBdAddr, &data[3], 6);
        pendingUserConfirmReply = true;
    }
    // 13. Classic BT Inquiry Result with RSSI (0x22) or Extended Inquiry (0x2F)
    else if ((eventCode == 0x22 || eventCode == 0x2F) && len >= 17) {
        int8_t rssi = (int8_t)data[16];
        if (rssi > -92 && rssi < 0) {
            if (rssi > windowPeakRssi) windowPeakRssi = rssi;
            lastPacketMs = millis();
        }
    }
    return 0;
}

static esp_vhci_host_callback_t vhci_host_cb = {
    controller_rcv_pkt_ready,
    host_rcv_pkt
};

static void sendHciCmd(const uint8_t* cmd, uint16_t len) {
    cmdCompleteReceived = false;
    for (int tries = 0; tries < 60; tries++) {
        if (esp_vhci_host_check_send_available()) {
            esp_vhci_host_send_packet((uint8_t*)cmd, len);
            break;
        }
        delay(2);
    }
    // Wait up to 40ms for controller Command Complete
    uint32_t t0 = millis();
    while (!cmdCompleteReceived && (millis() - t0 < 40)) {
        delay(2);
    }
}
#endif

RadarSystem radar;

void RadarSystem::begin() {
}

void RadarSystem::startScan() {
    scanning = true;
    scanStartTime = millis();
    smoothedRssi = -78.0f;
    proximity = 0.0f;

#ifdef WOKWI_SIM
    phonePaired = true;
    nextSimAlertTime = millis();
#else
    windowPeakRssi = -100;
    lastPacketMs = millis();
    lastBleScanTime = millis();
    pendingAcceptConn = false;
    pendingIoCapReply = false;
    pendingUserConfirmReply = false;
    pendingLinkKeyReply = false;
    pendingLinkKeyNegReply = false;
    pendingPinCodeReply = false;
    activeAclHandle = 0xFFFF;
    pairingDoneMs = 0;

    if (btStart()) {
        esp_vhci_host_register_callback(&vhci_host_cb);

        // 1. HCI Reset (Opcode 0x0C03)
        const uint8_t hciReset[4] = {0x01, 0x03, 0x0C, 0x00};
        sendHciCmd(hciReset, sizeof(hciReset));
        delay(50);

        // 2. Enable Simple Secure Pairing (SSP "Just Works" - NO PASSWORD/PIN REQUIRED! Opcode 0x0C56)
        const uint8_t hciSspMode[5] = {0x01, 0x56, 0x0C, 0x01, 0x01};
        sendHciCmd(hciSspMode, sizeof(hciSspMode));

        // 3. Disable legacy forced authentication on connect so SSP "Just Works" completes smoothly (Opcode 0x0C20)
        const uint8_t hciAuthDisable[5] = {0x01, 0x20, 0x0C, 0x01, 0x00};
        sendHciCmd(hciAuthDisable, sizeof(hciAuthDisable));

        // 4. HCI Set Event Mask (Opcode 0x0C01) - enables Bit 61 (LE Meta Event 0x3E) & all SSP events
        const uint8_t hciEventMask[12] = {
            0x01, 0x01, 0x0C, 0x08,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F
        };
        sendHciCmd(hciEventMask, sizeof(hciEventMask));

        // 5. HCI LE Set Event Mask (Opcode 0x2001)
        const uint8_t hciLeEventMask[12] = {
            0x01, 0x01, 0x20, 0x08,
            0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        sendHciCmd(hciLeEventMask, sizeof(hciLeEventMask));

        // 6. Classic BT: HCI Write Local Name "Portal-Gun-C137" (Opcode 0x0C13, 248-byte payload)
        static uint8_t hciLocalName[4 + 248];
        memset(hciLocalName, 0, sizeof(hciLocalName));
        hciLocalName[0] = 0x01;
        hciLocalName[1] = 0x13;
        hciLocalName[2] = 0x0C;
        hciLocalName[3] = 248;
        const char* btName = "Portal-Gun-C137";
        memcpy(&hciLocalName[4], btName, strlen(btName));
        sendHciCmd(hciLocalName, sizeof(hciLocalName));

        // 7. Classic BT: HCI Write Extended Inquiry Response (Opcode 0x0C52, 241-byte payload)
        static uint8_t hciEir[4 + 241];
        memset(hciEir, 0, sizeof(hciEir));
        hciEir[0] = 0x01;
        hciEir[1] = 0x52;
        hciEir[2] = 0x0C;
        hciEir[3] = 241;
        hciEir[4] = 0x00;
        hciEir[5] = 16;
        hciEir[6] = 0x09;
        memcpy(&hciEir[7], btName, 15);
        sendHciCmd(hciEir, sizeof(hciEir));

        // 8. Classic BT: HCI Write Class of Device (Opcode 0x0C24)
        // Wearable / Gadget Tracker class (0x04, 0x07, 0x00) so phone pairs without expecting A2DP audio!
        const uint8_t hciCod[7] = {0x01, 0x24, 0x0C, 0x03, 0x04, 0x07, 0x00};
        sendHciCmd(hciCod, sizeof(hciCod));

        // 9. Classic BT: HCI Write Inquiry Mode = 0x02 (Inquiry with RSSI / EIR, Opcode 0x0C45)
        const uint8_t hciInqMode[5] = {0x01, 0x45, 0x0C, 0x01, 0x02};
        sendHciCmd(hciInqMode, sizeof(hciInqMode));

        // 10. Classic BT: HCI Write Scan Enable (Opcode 0x0C1A, 0x03 = Inquiry + Page Scan)
        const uint8_t hciScanEnableClassic[5] = {0x01, 0x1A, 0x0C, 0x01, 0x03};
        sendHciCmd(hciScanEnableClassic, sizeof(hciScanEnableClassic));

        // 11. BLE Advertising Parameters (Opcode 0x2006)
        const uint8_t hciAdvParams[19] = {
            0x01, 0x06, 0x20, 0x0F,
            0x50, 0x00, 0x70, 0x00,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x07, 0x00
        };
        sendHciCmd(hciAdvParams, sizeof(hciAdvParams));

        // 12. BLE Advertising Data (Opcode 0x2008, 32-byte payload = 36 bytes total)
        uint8_t hciAdvData[36];
        memset(hciAdvData, 0, sizeof(hciAdvData));
        hciAdvData[0] = 0x01;
        hciAdvData[1] = 0x08;
        hciAdvData[2] = 0x20;
        hciAdvData[3] = 32;
        hciAdvData[4] = 20;
        hciAdvData[5] = 0x02;
        hciAdvData[6] = 0x01;
        hciAdvData[7] = 0x06;
        hciAdvData[8] = 16;
        hciAdvData[9] = 0x09;
        memcpy(&hciAdvData[10], btName, 15);
        sendHciCmd(hciAdvData, sizeof(hciAdvData));

        // 13. BLE Scan Response Data (Opcode 0x2009, 32-byte payload = 36 bytes total)
        uint8_t hciScanRsp[36];
        memset(hciScanRsp, 0, sizeof(hciScanRsp));
        hciScanRsp[0] = 0x01;
        hciScanRsp[1] = 0x09;
        hciScanRsp[2] = 0x20;
        hciScanRsp[3] = 32;
        hciScanRsp[4] = 17;
        hciScanRsp[5] = 16;
        hciScanRsp[6] = 0x09;
        memcpy(&hciScanRsp[7], btName, 15);
        sendHciCmd(hciScanRsp, sizeof(hciScanRsp));

        // 14. BLE Advertising Enable (Opcode 0x200A)
        const uint8_t hciAdvEnable[5] = {0x01, 0x0A, 0x20, 0x01, 0x01};
        sendHciCmd(hciAdvEnable, sizeof(hciAdvEnable));

        // 15. BLE Scan Parameters (Opcode 0x200B: Active scan, 50ms interval, 25ms window)
        // 50% duty cycle leaves plenty of radio time for Classic BT pairing & handshakes!
        const uint8_t hciScanParams[11] = {
            0x01, 0x0B, 0x20, 0x07,
            0x01,
            0x50, 0x00,
            0x28, 0x00,
            0x00, 0x00
        };
        sendHciCmd(hciScanParams, sizeof(hciScanParams));

        // 16. BLE Scan Enable (Opcode 0x200C: Enable = 1, Filter Duplicates = 0)
        const uint8_t hciScanEnable[6] = {0x01, 0x0C, 0x20, 0x02, 0x01, 0x00};
        sendHciCmd(hciScanEnable, sizeof(hciScanEnable));

        Serial.println("[BT Radar] Ready to pair 'Portal-Gun-C137' (SSP Just Works, no password needed)!");
    }
#endif
}

void RadarSystem::stopScan() {
    scanning = false;
#ifndef WOKWI_SIM
    if (btStarted()) {
        const uint8_t hciScanDisable[6] = {0x01, 0x0C, 0x20, 0x02, 0x00, 0x00};
        sendHciCmd(hciScanDisable, sizeof(hciScanDisable));
        const uint8_t hciAdvDisable[5] = {0x01, 0x0A, 0x20, 0x01, 0x00};
        sendHciCmd(hciAdvDisable, sizeof(hciAdvDisable));
        const uint8_t hciClassicDisable[5] = {0x01, 0x1A, 0x0C, 0x01, 0x00};
        sendHciCmd(hciClassicDisable, sizeof(hciClassicDisable));
    }
#endif
}

void RadarSystem::update() {
    if (!scanning) return;

    uint32_t now = millis();

#ifdef WOKWI_SIM
    phonePaired = true;
    proximity = (sinf(now / 1500.0f) + 1.0f) * 0.5f;
#else
    if (vPhonePaired) {
        phonePaired = true;
    }

    // Handle queued Classic BT Pairing / Connection handshakes
    if (pendingAcceptConn) {
        pendingAcceptConn = false;
        // Temporarily pause BLE scan during the 1-second pairing handshake so Classic BT has 100% radio priority
        const uint8_t hciScanDisable[6] = {0x01, 0x0C, 0x20, 0x02, 0x00, 0x00};
        sendHciCmd(hciScanDisable, sizeof(hciScanDisable));

        // HCI_Accept_Connection_Request (Opcode 0x0409, 7 bytes: BD_ADDR[6], Role=0x01 Slave)
        uint8_t cmd[11] = {0x01, 0x09, 0x04, 0x07};
        memcpy(&cmd[4], peerBdAddr, 6);
        cmd[10] = 0x01;
        sendHciCmd(cmd, sizeof(cmd));
        Serial.println("[BT Radar] Accepted phone connection request (paused BLE scan for clean pairing)!");
    }
    if (pendingLinkKeyReply) {
        pendingLinkKeyReply = false;
        // HCI_Link_Key_Request_Reply (Opcode 0x040B, 22 bytes: BD_ADDR[6] + Link_Key[16])
        uint8_t cmd[26] = {0x01, 0x0B, 0x04, 0x16};
        memcpy(&cmd[4], peerBdAddr, 6);
        memcpy(&cmd[10], savedLinkKey, 16);
        sendHciCmd(cmd, sizeof(cmd));
        Serial.println("[BT Radar] Sent saved Link Key reply!");
    }
    if (pendingLinkKeyNegReply) {
        pendingLinkKeyNegReply = false;
        // HCI_Link_Key_Request_Negative_Reply (Opcode 0x040C, 6 bytes: BD_ADDR[6])
        uint8_t cmd[10] = {0x01, 0x0C, 0x04, 0x06};
        memcpy(&cmd[4], peerBdAddr, 6);
        sendHciCmd(cmd, sizeof(cmd));
    }
    if (pendingPinCodeReply) {
        pendingPinCodeReply = false;
        // HCI_PIN_Code_Request_Reply (Opcode 0x040D, 23 bytes: BD_ADDR[6], len=4, "0000")
        uint8_t cmd[27];
        memset(cmd, 0, sizeof(cmd));
        cmd[0] = 0x01; cmd[1] = 0x0D; cmd[2] = 0x04; cmd[3] = 23;
        memcpy(&cmd[4], peerBdAddr, 6);
        cmd[10] = 4;
        memcpy(&cmd[11], "0000", 4);
        sendHciCmd(cmd, sizeof(cmd));
    }
    if (pendingIoCapReply) {
        pendingIoCapReply = false;
        // HCI_IO_Capability_Request_Reply (Opcode 0x042B, 9 bytes: BD_ADDR[6], NoInputNoOutput=0x03, OOB=0, Auth=0x02 Dedicated Bonding)
        uint8_t cmd[13] = {0x01, 0x2B, 0x04, 0x09};
        memcpy(&cmd[4], peerBdAddr, 6);
        cmd[10] = 0x03; // NoInputNoOutput ("Just Works" - no password prompt!)
        cmd[11] = 0x00; // OOB Data not present
        cmd[12] = 0x02; // Dedicated Bonding - MITM Protection Not Required
        sendHciCmd(cmd, sizeof(cmd));
        Serial.println("[BT Radar] Sent SSP Just-Works IO Capability Reply!");
    }
    if (pendingUserConfirmReply) {
        pendingUserConfirmReply = false;
        // HCI_User_Confirmation_Request_Reply (Opcode 0x042C, 6 bytes: BD_ADDR[6])
        uint8_t cmd[10] = {0x01, 0x2C, 0x04, 0x06};
        memcpy(&cmd[4], peerBdAddr, 6);
        sendHciCmd(cmd, sizeof(cmd));
        Serial.println("[BT Radar] Auto-confirmed SSP User Confirmation Request!");
    }

    // Once pairing has completed (pairingDoneMs > 0), wait 1.5s for phone OS to save bond, then resume full BLE RSSI scan!
    if (pairingDoneMs != 0 && (now - pairingDoneMs > 1500)) {
        pairingDoneMs = 0;
        if (activeAclHandle != 0xFFFF) {
            uint16_t h = activeAclHandle;
            activeAclHandle = 0xFFFF;
            uint8_t discCmd[7] = {
                0x01, 0x06, 0x04, 0x03,
                (uint8_t)(h & 0xFF), (uint8_t)((h >> 8) & 0xFF),
                0x13
            };
            sendHciCmd(discCmd, sizeof(discCmd));
        }
        const uint8_t hciScanEnable[6] = {0x01, 0x0C, 0x20, 0x02, 0x01, 0x00};
        sendHciCmd(hciScanEnable, sizeof(hciScanEnable));
        Serial.println("[BT Radar] Pairing bond saved! Resumed full-speed BLE RSSI proximity tracking.");
    }

    // Update smoothed RSSI & proximity (0.0 = far/green, 1.0 = very close/red) every 180ms
    if (now - lastBleScanTime >= 180) {
        lastBleScanTime = now;
        int8_t peak = windowPeakRssi;
        windowPeakRssi = -100; // Reset window peak for next 180ms slice

        float targetRssi;
        if (peak > -92) {
            targetRssi = (float)peak;
            // Fast response when signal gets stronger, smooth decay when moving away
            float alpha = (targetRssi > smoothedRssi) ? 0.45f : 0.22f;
            smoothedRssi += (targetRssi - smoothedRssi) * alpha;
        } else if (now - lastPacketMs > 800) {
            // No packets recently -> decay gently toward far (-85 dBm)
            smoothedRssi += (-85.0f - smoothedRssi) * 0.15f;
        }

        // Map smoothedRssi from [-80 dBm (far) .. -42 dBm (right next to Portal Gun)] -> [0.0f .. 1.0f]
        float norm = (smoothedRssi - (-80.0f)) / ((-42.0f) - (-80.0f));
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        proximity = norm;
    }
#endif
}
