#pragma once
#include "app_state.h"
#include <stdint.h>
#include <stddef.h>

class PortalGun {
public:
    void begin();
    void update();

private:
    PortalState currentState = PortalState::PowerUp;
    
    void changeState(PortalState newState);
    
    // State handlers
    void handlePowerUp();
    void handlePowerUpFinalize();
    void handleDimensionSelection();
    void handleParty();
    void handlePowerDown();
    void handleDeepSleep();

    uint32_t stateStartTime = 0;
    bool stateInit = false;
    
    // Subtitle tracking
    size_t currentLyricIndex = 0;
    bool isLyricScrolling = false;
};

extern PortalGun portalGun;
