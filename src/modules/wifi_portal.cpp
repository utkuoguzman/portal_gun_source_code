#include "wifi_portal.h"
#include "web_page.h"
#include "site_assets.h"
#include "portal_gun.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

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
        server.send(200, "application/json", "{\"success\":true,\"message\":\"PORTAL BLAST ACTIVATED!\"}");
    });

    server.on("/api/close", HTTP_GET, []() {
        portalGun.triggerWebClose();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"PORTAL CLOSED!\"}");
    });

    server.on("/api/party", HTTP_GET, []() {
        portalGun.triggerWebParty();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"GET SCHWIFTY ACTIVATED!\"}");
    });

    server.on("/api/quote", HTTP_GET, []() {
        portalGun.triggerWebQuote();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"RICK VOICE TRANSMITTED!\"}");
    });

    server.on("/api/torch", HTTP_GET, []() {
        portalGun.triggerWebTorch();
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

    // Catch-all: redirect any arbitrary HTTP request to captive portal
    server.onNotFound([]() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
}

void WifiPortalSystem::startPortal() {
    if (active) return;

    WiFi.mode(WIFI_AP);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Portal-Gun-C137");

    dnsServer.start(DNS_PORT, "*", apIP);
    setupRoutes();
    server.begin();
    active = true;
    Serial.println("Wi-Fi Captive Portal started: Portal-Gun-C137 at 192.168.4.1");
}

void WifiPortalSystem::stopPortal() {
    if (!active) return;
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
}

bool WifiPortalSystem::isActive() const {
    return active;
}

int WifiPortalSystem::getClientCount() const {
    if (!active) return 0;
    return WiFi.softAPgetStationNum();
}

