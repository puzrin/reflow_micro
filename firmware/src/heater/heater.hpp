#pragma once

#if defined(RFL_USE_MOCKS)
#include "heater_control_mock.hpp"
using Heater = HeaterControlMock;
#else
#include "heater_control.hpp"
using Heater = HeaterControl;
#endif

inline Heater heater;
