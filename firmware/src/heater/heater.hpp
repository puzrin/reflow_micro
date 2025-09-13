#pragma once

#if defined(HW_DEMO_ESP32_C3_SUPERMINI)
#include "heater_control_mock.hpp"
using Heater = HeaterControlMock;
#else
#include "heater_control.hpp"
using Heater = HeaterControl;
#endif

inline Heater heater;
