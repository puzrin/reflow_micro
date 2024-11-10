#pragma once

#include "blinker/blinker.h"
#include "button/button.hpp"

#define BLINK_IDLE_BACKGROUND { 10 }

#define BLINK_LONG_PRESS_START { {10, 0}, blinker.flowTo(255, Button::LONG_PRESS_THRESHOLD - Button::SHORT_PRESS_THRESHOLD) }

#define BLINK_BONDING_LOOP { {255, 150}, {0, 250} }
