#pragma once

#if defined(HW_DEMO_ESP32_C3_SUPERMINI)
#include "heater_control_mock.hpp"
using Heater = HeaterControlMock;
#else
// temporary stub
#include "heater_control_mock.hpp"
using Heater = HeaterControlMock;
#endif

inline Heater heater;
