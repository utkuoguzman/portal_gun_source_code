#pragma once
#include "app_state.h"
#include <stdint.h>
#include <stddef.h>

class PortalGun {
public:
    void begin();
    void update();

    void triggerWebFire();
    void triggerWebClose();
    void triggerWebQuote();
    void triggerWebTorch();
    void triggerWebParty();
    void setWebDimension(char letter, int number);
    void setWebColor(uint8_t r, uint8_t g, uint8_t b);

private:
    PortalState currentState = PortalState::PowerUp;
    
    void changeState(PortalState newState);
    
    // State handlers
    void handlePowerUp();
    void handlePowerUpFinalize();
    void handleDimensionSelection();
    void handleParty();
    void handleTorch();
    void handleRadarScan();
    void handleWifiPortal();
    void handleEvilMorty();
    void handleSnakeGame();
    void handleRoulette();
    void handleDinoGame();
    void handleRickQuote();
    void handlePowerDown();
    void handleDeepSleep();

    uint32_t stateStartTime = 0;
    bool stateInit = false;
    
    // Subtitle tracking
    size_t currentLyricIndex = 0;
    bool isLyricScrolling = false;

    // Rick soundboard quote tracking
    bool isQuoteActive = false;
    bool quoteAudioStarted = false;
    int currentQuoteIdx = 0;
};

extern PortalGun portalGun;
