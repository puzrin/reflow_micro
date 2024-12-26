#include "app.hpp"
#include "logger.hpp"
#include "blink_signals.hpp"

const etl::message_router_id_t APP_FSM_ROUTER_ID = 0;
App app;

App::App() : etl::fsm(APP_FSM_ROUTER_ID) {
    button.setEventHandler([this](ButtonEventId event) {
        this->handleButton(event);
    });
}

void App::setup() {
    app_setup_states(*this);
    start();
    button.start();
    blinker.start();
    BLINK_SET_IDLE_BACKGROUND(blinker);

}

void App::LogUnknownEvent(const etl::imessage& msg) {
    DEBUG("APP: Unknown event! msg id [{}], state id [{}]", int(msg.get_message_id()), int(get_state_id()));
}

