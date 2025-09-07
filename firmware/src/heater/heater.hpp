#pragma once

#if defined(HW_DEMO_ESP32_C3_SUPERMINI)
#include "heater_mock.hpp"
inline HeaterMock heater;
#else
// temporary stub
#include "heater_mock.hpp"
inline HeaterMock heater;
#endif
