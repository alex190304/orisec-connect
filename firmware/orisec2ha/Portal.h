#pragma once
#include <Arduino.h>
#include "Globals.h"

String makeApPassword();

void startConfigPortal();
void stopConfigPortal();
void configPortalLoop();
