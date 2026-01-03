#pragma once
#include <Arduino.h>

void initStatusLeds();
void pulseTxLed();
void pulseRxLed();
void updateStatusLeds();
void setFactoryResetActive(bool active);
