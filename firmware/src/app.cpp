#include "app.hpp"
#include "logger.hpp"

const etl::message_router_id_t APP_FSM_ROUTER_ID = 0;

App::App() : etl::fsm(APP_FSM_ROUTER_ID) {}

void App::LogUnknownEvent(const etl::imessage& msg) {
    DEBUG("APP: Unknown event! msg id [{}], state id [{}]", int(msg.get_message_id()), int(get_state_id()));
}

App app;

void app_init() {
    app_states_init(app);
    app.start();
}

