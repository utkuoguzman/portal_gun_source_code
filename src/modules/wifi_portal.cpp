#include "wifi_portal.h"
#include "web_page.h"
#include "web_ota.h"
#include "site_assets.h"
#include "portal_gun.h"
#include "display.h"
#include "leds.h"
#include "naked_test.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>

WifiPortalSystem wifiPortal;

static DNSServer dnsServer;
static WebServer server(80);
static const byte DNS_PORT = 53;

void WifiPortalSystem::begin() {
    active = false;
    // Keep WiFi off by default until triggered
    WiFi.mode(WIFI_OFF);
}

void WifiPortalSystem::setupRoutes() {
    auto handleCaptive = []() {
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.send_P(200, "text/html", PAGE_INDEX);
    };

    // Captive portal detection endpoints
    server.on("/", HTTP_GET, handleCaptive);
    server.on("/hotspot-detect.html", HTTP_GET, handleCaptive); // iOS
    server.on("/generate_204", HTTP_GET, handleCaptive);        // Android
    server.on("/gen_204", HTTP_GET, handleCaptive);             // Android
    server.on("/canonical.html", HTTP_GET, handleCaptive);      // Windows
    server.on("/ncsi.txt", HTTP_GET, handleCaptive);            // Windows
    server.on("/connecttest.txt", HTTP_GET, handleCaptive);
    server.on("/redirect", HTTP_GET, handleCaptive);

    // Static Assets: Portal Background & Rick Dance GIF
    auto handleBg = []() {
        server.sendHeader("Cache-Control", "public, max-age=3600");
        server.send_P(200, "image/jpeg", (const char*)portal_background_jpg, portal_background_jpg_size);
    };
    server.on("/portal_background.jpg", HTTP_GET, handleBg);
    server.on("/site_assets/portal_background.jpg", HTTP_GET, handleBg);

    auto handleGif = []() {
        server.sendHeader("Cache-Control", "public, max-age=3600");
        server.send_P(200, "image/gif", (const char*)rick_dance_gif, rick_dance_gif_size);
    };
    server.on("/rick-dance.gif", HTTP_GET, handleGif);
    server.on("/site_assets/rick-dance.gif", HTTP_GET, handleGif);

    // 2-Way Hardware API Endpoints
    server.on("/api/fire", HTTP_GET, []() {
        portalGun.triggerWebFire();
#ifdef NAKED_TEST_MODE
        setOnboardRgb(0, 255, 60);
        Serial.println("[WEB] 💥 PORTAL BLAST TRIGGERED FROM PHONE!");
#endif
        server.send(200, "application/json", "{\"success\":true,\"message\":\"PORTAL BLAST ACTIVATED!\"}");
    });

    server.on("/api/close", HTTP_GET, []() {
        portalGun.triggerWebClose();
#ifdef NAKED_TEST_MODE
        setOnboardRgb(0, 30, 10);
        Serial.println("[WEB] 🚪 PORTAL CLOSED FROM PHONE!");
#endif
        server.send(200, "application/json", "{\"success\":true,\"message\":\"PORTAL CLOSED!\"}");
    });

    server.on("/api/party", HTTP_GET, []() {
        portalGun.triggerWebParty();
#ifdef NAKED_TEST_MODE
        Serial.println("[WEB] 🎉 PARTY MODE (Get Schwifty) TRIGGERED FROM PHONE!");
#endif
        server.send(200, "application/json", "{\"success\":true,\"message\":\"GET SCHWIFTY ACTIVATED!\"}");
    });

    server.on("/api/quote", HTTP_GET, []() {
        portalGun.triggerWebQuote();
#ifdef NAKED_TEST_MODE
        Serial.println("[WEB] 🎙️ RICK VOICE QUOTE TRIGGERED FROM PHONE!");
#endif
        server.send(200, "application/json", "{\"success\":true,\"message\":\"RICK VOICE TRANSMITTED!\"}");
    });

    server.on("/api/torch", HTTP_GET, []() {
        portalGun.triggerWebTorch();
#ifdef NAKED_TEST_MODE
        setOnboardRgb(255, 255, 255);
        Serial.println("[WEB] 🔦 TORCH / FLASHLIGHT TRIGGERED FROM PHONE!");
#endif
        server.send(200, "application/json", "{\"success\":true,\"message\":\"TORCH BEAM TOGGLED!\"}");
    });

    server.on("/api/dimension", HTTP_GET, []() {
        if (server.hasArg("d")) {
            String dim = server.arg("d");
            dim.trim();
            dim.toUpperCase();
            if (dim.length() >= 2) {
                char l = dim.charAt(0);
                // Support both "C137" and "C-137"
                int numStart = (dim.charAt(1) == '-') ? 2 : 1;
                int n = dim.substring(numStart).toInt();
                portalGun.setWebDimension(l, n);
#ifdef NAKED_TEST_MODE
                Serial.printf("[WEB] 🌌 DIMENSION LOCKED VIA PHONE: %s\n", dim.c_str());
#endif
                String res = "{\"success\":true,\"message\":\"DIMENSION LOCKED: " + dim + "\",\"dimension\":\"" + dim + "\"}";
                server.send(200, "application/json", res);
                return;
            }
        }
        server.send(400, "application/json", "{\"success\":false,\"message\":\"INVALID DIMENSION CODE\"}");
    });

    server.on("/api/color", HTTP_GET, []() {
        if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
            int r = server.arg("r").toInt();
            int g = server.arg("g").toInt();
            int b = server.arg("b").toInt();
            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;
            portalGun.setWebColor((uint8_t)r, (uint8_t)g, (uint8_t)b);
#ifdef NAKED_TEST_MODE
            setOnboardRgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
            Serial.printf("[WEB] 🎨 Color tuned via phone slider: RGB(%d, %d, %d)\n", r, g, b);
#endif
            char res[80];
            snprintf(res, sizeof(res), "{\"success\":true,\"message\":\"COLOR TUNED\",\"r\":%d,\"g\":%d,\"b\":%d}", r, g, b);
            server.send(200, "application/json", res);
            return;
        }
        server.send(400, "application/json", "{\"success\":false,\"message\":\"MISSING RGB ARGS\"}");
    });

    server.on("/api/status", HTTP_GET, []() {
        server.send(200, "application/json", "{\"success\":true,\"active\":true,\"device\":\"Portal-Gun-C137\"}");
    });

    // OTA Firmware Update Endpoints
    server.on("/update", HTTP_GET, []() {
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.send_P(200, "text/html", PAGE_UPDATE);
    });

    server.on("/api/ota_info", HTTP_GET, []() {
        const esp_partition_t* running = esp_ota_get_running_partition();
        const char* part_label = running ? running->label : "app0";
        char buf[256];
        snprintf(buf, sizeof(buf), 
            "{\"chip\":\"ESP32 DevKit V1\",\"partition\":\"%s\",\"flash_size\":4,\"free_heap\":%u,\"sketch_size\":%u,\"free_sketch_space\":%u}",
            part_label, ESP.getFreeHeap(), ESP.getSketchSize(), ESP.getFreeSketchSpace());
        server.send(200, "application/json", buf);
    });

    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        if (Update.hasError()) {
            server.send(500, "application/json", "{\"success\":false,\"error\":\"OTA Flash Verification Failed\"}");
            display.showText("ERR_");
        } else {
            server.send(200, "application/json", "{\"success\":true,\"message\":\"FIRMWARE FLASHED SUCCESSFULLY!\"}");
            display.showText("DONE");
            delay(1000);
            ESP.restart();
        }
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[OTA] Updating firmware from %s\n", upload.filename.c_str());
            display.showText("UPDT");
            leds.setAllPortalColor(0, 200, 255);
            leds.showPortal();
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                Update.printError(Serial);
                display.showText("ERR_");
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
                display.showText("ERR_");
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Successfully written: %u bytes\n", upload.totalSize);
                display.showText("DONE");
            } else {
                Update.printError(Serial);
                display.showText("ERR_");
            }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            Update.end();
            Serial.println("[OTA] Upload Aborted!");
            display.showText("ABRT");
        }
    });

    // Catch-all: redirect any arbitrary HTTP request to captive portal
    server.onNotFound([]() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
}

void WifiPortalSystem::startPortal() {
    if (active) return;

    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    delay(50);

    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    bool apStarted = WiFi.softAP("Portal-Gun-C137", nullptr, 1, 0, 4);
    delay(100);
    WiFi.setTxPower(WIFI_POWER_17dBm);

    dnsServer.start(DNS_PORT, "*", apIP);
    setupRoutes();
    server.begin();

    // Setup ArduinoOTA for IDE/PlatformIO direct wireless flashing
    ArduinoOTA.setHostname("Portal-Gun-C137");
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
        Serial.println("[ArduinoOTA] Start updating " + type);
        display.showText("UPDT");
        leds.setAllPortalColor(0, 200, 255);
        leds.showPortal();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total > 0) {
            int pct = (progress * 100) / total;
            display.showNumber(pct);
        }
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[ArduinoOTA] Update complete. Rebooting...");
        display.showText("DONE");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA] Error[%u]\n", error);
        display.showText("ERR_");
    });
    ArduinoOTA.begin();

    active = true;
    Serial.printf("Wi-Fi Captive Portal & OTA started (status=%d): Portal-Gun-C137 at 192.168.4.1\n", apStarted);
}

void WifiPortalSystem::stopPortal() {
    if (!active) return;
    ArduinoOTA.end();
    server.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    active = false;
    Serial.println("Wi-Fi Captive Portal stopped. WiFi turned off.");
}

void WifiPortalSystem::update() {
    if (!active) return;
    dnsServer.processNextRequest();
    server.handleClient();
    ArduinoOTA.handle();
}

bool WifiPortalSystem::isActive() const {
    return active;
}

int WifiPortalSystem::getClientCount() const {
    if (!active) return 0;
    return WiFi.softAPgetStationNum();
}

