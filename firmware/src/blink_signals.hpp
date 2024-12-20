#pragma once

#include "components/blinker.hpp"
#include "lib/button_engine.hpp"

#define BLINK_SET_IDLE_BACKGROUND(blnkr) (blnkr).background({ 10 })

#define BLINK_LONG_PRESS_START(blnkr) (blnkr).once({ {10, 0}, (blnkr).flowTo(255, ButtonConstants::LONG_PRESS_THRESHOLD - ButtonConstants::SHORT_PRESS_THRESHOLD) })

#define BLINK_BONDING_LOOP(blnkr) (blnkr).loop({ {255, 150}, {0, 250} })
