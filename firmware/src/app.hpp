#pragma once

#include "etl/fsm.h"
#include "button/button.hpp"
#include "heater_mock.hpp"

struct AppEventId {
    enum Enum {
        START,
        STOP,
        BOND_ON,
        BOND_OFF,
        BUTTON_ACTION,
    };
};

class Start : public etl::message<AppEventId::START> {};
class Stop : public etl::message<AppEventId::STOP> {};
class BondOn : public etl::message<AppEventId::BOND_ON> {};
class BondOff : public etl::message<AppEventId::BOND_OFF> {};

class ButtonAction : public etl::message<AppEventId::BUTTON_ACTION> {
public:
    ButtonAction(ButtonEventId type) : type(type) {}
    ButtonEventId type;
};


class App : public etl::fsm {
public:
    App();

    HeaterMock heater;

    void LogUnknownEvent(const etl::imessage& msg);
};

extern App app;

void app_init();
void app_states_init(App& app);

