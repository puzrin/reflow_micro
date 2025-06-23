#pragma once

#include "components/blinker.hpp"
#include "lib/button_engine.hpp"

#ifdef HW_DEMO_ESP32_C3_SUPERMINI

#define BLINK_SET_IDLE_BACKGROUND(blnkr) (blnkr).background({10})
#define BLINK_LONG_PRESS_START(blnkr) (blnkr).once({{10, 0}, (blnkr).flowTo(255, ButtonConstants::LONG_PRESS_THRESHOLD - ButtonConstants::SHORT_PRESS_THRESHOLD) })
#define BLINK_BONDING_LOOP(blnkr) (blnkr).loop({ {255, 150}, {0, 250} })
#define BLINK_REFLOW_START(blnkr) (blnkr).once({{0, 200}, {255, 300}, {0, 200}})

#define BLINK_TEST(blnkr)

#else

#define BLINK_SET_IDLE_BACKGROUND(blnkr) (blnkr).background({0,100,0})
#define BLINK_LONG_PRESS_START(blnkr) (blnkr).once({{{0,100,0}, 0}, (blnkr).flowTo({255,255,255}, ButtonConstants::LONG_PRESS_THRESHOLD - ButtonConstants::SHORT_PRESS_THRESHOLD)})
#define BLINK_BONDING_LOOP(blnkr) (blnkr).loop({{{0,0,255}, 150}, {{0,0,0}, 250}})
#define BLINK_REFLOW_START(blnkr) (blnkr).once({{{0,0,0}, 200}, {{0,255,0}, 300}, {{0,0,0}, 200}})

#define BLINK_TEST(blnkr) (blnkr).loop({{{255,0,0}, 1000}, {{0,255,0}, 1000}, {{0,0,255}, 1000}, {{128,128,128}, 1000}})

#endif // HW_DEMO_ESP32_C3_SUPERMINI