#pragma once
#include <Arduino.h>

#ifdef NAKED_TEST_MODE

void setupNakedTest();
void loopNakedTest();
void setOnboardRgb(uint8_t r, uint8_t g, uint8_t b);

#endif

