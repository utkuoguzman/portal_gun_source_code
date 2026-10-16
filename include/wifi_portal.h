#pragma once
#include <Arduino.h>

class WifiPortalSystem {
public:
    void begin();
    void startPortal();
    void stopPortal();
    void update();
    bool isActive() const;
    int getClientCount() const;

private:
    bool active = false;
    void setupRoutes();
};

extern WifiPortalSystem wifiPortal;

