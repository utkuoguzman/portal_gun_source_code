#include "naked_test.h"

#ifdef NAKED_TEST_MODE

// Minimal includes for Wi-Fi diagnosis
#include "portal_gun.h"
#include "leds.h"
#include "display.h"
#include "soundboard_data.h"
#include "extra_audio.h"
#include "audio_assets.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// Dummy audio buffers to keep Naked Test Mode binary under 1.2MB
const uint8_t snd_powerup[1] = {0}; const size_t snd_powerup_size = 0;
const uint8_t snd_buckleup[1] = {0}; const size_t snd_buckleup_size = 0;
const uint8_t snd_portal_open1[1] = {0}; const size_t snd_portal_open1_size = 0;
const uint8_t snd_portal_open2[1] = {0}; const size_t snd_portal_open2_size = 0;
const uint8_t snd_portal_close1[1] = {0}; const size_t snd_portal_close1_size = 0;
const uint8_t snd_portal_close2[1] = {0}; const size_t snd_portal_close2_size = 0;
const uint8_t snd_getschwifty[1] = {0}; const size_t snd_getschwifty_size = 0;
const uint8_t env_getschwifty_low[1] = {0}; const size_t env_getschwifty_low_size = 0;
const uint8_t env_getschwifty_mid[1] = {0}; const size_t env_getschwifty_mid_size = 0;
const uint8_t env_getschwifty_high[1] = {0}; const size_t env_getschwifty_high_size = 0;
const SubtitleLine getschwifty_lyrics[1] = {{0, 0, ""}};
const Soundbyte rick_soundboard[1] = {{nullptr, 0, "NONE"}};
const size_t rick_soundboard_count = 0;

const uint8_t snd_evil_morty[1] = {0}; const size_t snd_evil_morty_size = 0;
const uint8_t snd_roulette_spin[1] = {0}; const size_t snd_roulette_spin_size = 0;
const uint8_t snd_roulette_clack[1] = {0}; const size_t snd_roulette_clack_size = 0;
const uint8_t snd_roulette_boom[1] = {0}; const size_t snd_roulette_boom_size = 0;
const uint8_t snd_roulette_wrecked[1] = {0}; const size_t snd_roulette_wrecked_size = 0;
const uint8_t snd_snake_jazz[1] = {0}; const size_t snd_snake_jazz_size = 0;
const uint8_t snd_dino_song[1] = {0}; const size_t snd_dino_song_size = 0;

// Onboard RGB pins for Arduino Nano ESP32
#ifndef LED_RED
#define LED_RED 46
#endif
#ifndef LED_GREEN
#define LED_GREEN 0
#endif
#ifndef LED_BLUE
#define LED_BLUE 45
#endif

static uint32_t lastClientReport = 0;
static int lastKnownClients = -1;

// Current LED color
static uint8_t curR = 0, curG = 0, curB = 0;
static bool pulseEffect = true;

// Standalone minimal web server (no external modules)
static WebServer nakedServer(80);
static DNSServer nakedDns;

void setOnboardRgb(uint8_t r, uint8_t g, uint8_t b) {
    curR = r; curG = g; curB = b;
    pulseEffect = false;
    // Arduino Nano ESP32 RGB is active-LOW (common anode)
    analogWrite(LED_RED,   255 - r);
    analogWrite(LED_GREEN, 255 - g);
    analogWrite(LED_BLUE,  255 - b);
}

// Minimal HTML page embedded directly here
static const char MINIMAL_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Portal Gun C-137</title>
<style>
body{background:#0a0a0a;color:#0f0;font-family:monospace;text-align:center;padding:20px}
h1{color:#0f0;font-size:1.5em}
.btn{display:inline-block;margin:8px;padding:15px 25px;border:2px solid #0f0;background:#111;
color:#0f0;font-size:1.1em;cursor:pointer;border-radius:8px;font-family:monospace}
.btn:active{background:#0f0;color:#000}
#status{margin:20px;padding:10px;border:1px solid #333;min-height:30px}
input[type=range]{width:80%;margin:10px}
</style></head><body>
<h1>&#x1F52B; Portal Gun C-137 &#x1F52B;</h1>
<p>Wi-Fi Test Bench - Arduino Nano ESP32</p>
<div>
<button class="btn" onclick="c(255,0,0)">RED</button>
<button class="btn" onclick="c(0,255,0)">GREEN</button>
<button class="btn" onclick="c(0,100,255)">BLUE</button>
<button class="btn" onclick="c(255,230,0)">MORTY</button>
<button class="btn" onclick="c(0,220,255)">UNITY</button>
<button class="btn" onclick="c(255,255,255)">RICK</button>
</div>
<div><button class="btn" style="border-color:red;color:red" onclick="fire()">FIRE PORTAL</button></div>
<p>Hue: <input type="range" id="hue" min="0" max="360" value="120" oninput="hueChange(this.value)"></p>
<div id="status">Ready</div>
<script>
function c(r,g,b){fetch('/api/color?r='+r+'&g='+g+'&b='+b).then(r=>r.json()).then(j=>{
document.getElementById('status').textContent='Color: RGB('+j.r+','+j.g+','+j.b+')';});}
function fire(){fetch('/api/fire').then(()=>{document.getElementById('status').textContent='PORTAL BLAST!';});}
function hueChange(h){var s=1,v=1;h=h/60;var i=Math.floor(h),f=h-i,p=0,q=v*(1-f),t=v*f;
var r,g,b;switch(i%6){case 0:r=v;g=t;b=p;break;case 1:r=q;g=v;b=p;break;
case 2:r=p;g=v;b=t;break;case 3:r=p;g=q;b=v;break;case 4:r=t;g=p;b=v;break;
case 5:r=v;g=p;b=q;break;}c(Math.round(r*255),Math.round(g*255),Math.round(b*255));}
</script></body></html>)rawliteral";

void setupNakedTest() {
    Serial.begin(115200);
    delay(2000);  // Extra time for USB CDC to stabilize

    // Initialize onboard RGB LED
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    
    // Quick startup LED sequence: Red -> Green -> Blue -> Off
    analogWrite(LED_RED, 0);   analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 255); delay(200);
    analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 0);   analogWrite(LED_BLUE, 255); delay(200);
    analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 0);   delay(200);
    analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 255); delay(100);

    // Banner
    Serial.println();
    Serial.println("==================================================================");
    Serial.println("  PORTAL GUN C-137 - MINIMAL WIFI DIAGNOSTIC TEST");
    Serial.println("==================================================================");
    Serial.println("[SYSTEM] Board: Arduino Nano ESP32 (ESP32-S3)");
    Serial.println();
    
    // ---- STEP 1: Wi-Fi Mode ----
    Serial.println("[WIFI] Step 1: WiFi.mode(WIFI_AP)...");
    WiFi.mode(WIFI_AP);
    delay(200);
    Serial.println("[WIFI] Step 1: OK");

    // ---- STEP 2: softAPConfig ----
    Serial.println("[WIFI] Step 2: WiFi.softAPConfig...");
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    delay(100);
    Serial.println("[WIFI] Step 2: OK");

    // ---- STEP 3: softAP ----
    Serial.println("[WIFI] Step 3: WiFi.softAP(Portal-Gun-C137)...");
    bool ok = WiFi.softAP("Portal-Gun-C137", NULL, 1, 0, 4);
    delay(500);
    Serial.printf("[WIFI] Step 3: softAP returned %s\n", ok ? "TRUE" : "FALSE");
    Serial.printf("[WIFI] AP IP: %s\n", WiFi.softAPIP().toString().c_str());

    // ---- STEP 4: DNS Server ----
    Serial.println("[WIFI] Step 4: Starting DNS captive portal...");
    nakedDns.start(53, "*", apIP);
    Serial.println("[WIFI] Step 4: OK");

    // ---- STEP 5: Web Server Routes ----
    Serial.println("[WIFI] Step 5: Setting up web server routes...");
    
    nakedServer.on("/", HTTP_GET, []() {
        nakedServer.send_P(200, "text/html", MINIMAL_PAGE);
    });
    nakedServer.on("/generate_204", HTTP_GET, []() {
        nakedServer.send_P(200, "text/html", MINIMAL_PAGE);
    });
    nakedServer.on("/hotspot-detect.html", HTTP_GET, []() {
        nakedServer.send_P(200, "text/html", MINIMAL_PAGE);
    });
    
    nakedServer.on("/api/color", HTTP_GET, []() {
        if (nakedServer.hasArg("r") && nakedServer.hasArg("g") && nakedServer.hasArg("b")) {
            uint8_t r = constrain(nakedServer.arg("r").toInt(), 0, 255);
            uint8_t g = constrain(nakedServer.arg("g").toInt(), 0, 255);
            uint8_t b = constrain(nakedServer.arg("b").toInt(), 0, 255);
            setOnboardRgb(r, g, b);
            Serial.printf("[WEB] Color: RGB(%d,%d,%d)\n", r, g, b);
            char buf[100];
            snprintf(buf, sizeof(buf), "{\"success\":true,\"r\":%d,\"g\":%d,\"b\":%d}", r, g, b);
            nakedServer.send(200, "application/json", buf);
        } else {
            nakedServer.send(400, "application/json", "{\"error\":\"need r,g,b\"}");
        }
    });

    nakedServer.on("/api/fire", HTTP_GET, []() {
        Serial.println("[WEB] FIRE PORTAL!");
        nakedServer.send(200, "application/json", "{\"success\":true}");
        for (int i = 0; i < 5; i++) {
            setOnboardRgb(0, 255, 50); delay(60);
            setOnboardRgb(20, 80, 20); delay(60);
        }
        setOnboardRgb(0, 255, 50);
    });

    nakedServer.on("/api/status", HTTP_GET, []() {
        nakedServer.send(200, "application/json", "{\"alive\":true,\"device\":\"Portal-Gun-C137\"}");
    });

    nakedServer.onNotFound([]() {
        nakedServer.sendHeader("Location", "http://192.168.4.1/", true);
        nakedServer.send(302, "text/plain", "");
    });

    nakedServer.begin();
    Serial.println("[WIFI] Step 5: OK - Web server started");

    Serial.println();
    Serial.println("==================================================================");
    Serial.println("  ALL SYSTEMS GO!");
    Serial.println("  SSID: Portal-Gun-C137  (open, no password)");
    Serial.println("  URL:  http://192.168.4.1");
    Serial.println("==================================================================");
    Serial.println();

    // Default idle color: soft Rick green
    setOnboardRgb(0, 80, 20);
    pulseEffect = true;
}

void loopNakedTest() {
    // Process DNS & HTTP
    nakedDns.processNextRequest();
    nakedServer.handleClient();

    // Gentle idle breathing
    if (pulseEffect) {
        uint32_t now = millis();
        float val = (sin(now / 400.0f) + 1.0f) * 0.5f;
        uint8_t g = (uint8_t)(val * 90.0f + 10.0f);
        uint8_t b = (uint8_t)(val * 25.0f);
        analogWrite(LED_RED, 255);
        analogWrite(LED_GREEN, 255 - g);
        analogWrite(LED_BLUE, 255 - b);
    }
}

#endif

