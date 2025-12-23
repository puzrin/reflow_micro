#pragma once

#include <etl/atomic.h>
#include <etl/fsm.h>
#include <etl/limits.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <pd/pd.h>

#include "profile_selector.hpp"
#include "proto/generated/types.pb.h"
#include "pwm.hpp"

// Extra custom events
DEFINE_SIMPLE_MSG(MsgToPower_SysTick, 200);

using DPM_EventListener_Base = etl::message_router<class DPM_EventListener,
    pd::MsgToDpm_Startup,
    pd::MsgToDpm_TransitToDefault,
    pd::MsgToDpm_SrcCapsReceived,
    pd::MsgToDpm_SelectCapDone,
    pd::MsgToDpm_SrcDisabled,
    pd::MsgToDpm_Alert,
    pd::MsgToDpm_SnkReady,
    pd::MsgToDpm_CableDetached,
    pd::MsgToDpm_HandshakeDone,
    pd::MsgToDpm_NewPowerLevelRejected
>;

extern ProfileSelector profile_selector;

class DPM_EventListener : public DPM_EventListener_Base {
public:
    DPM_EventListener(class Power& power) : power(power) {}
    void on_receive(const pd::MsgToDpm_Startup& msg);
    void on_receive(const pd::MsgToDpm_TransitToDefault& msg);
    void on_receive(const pd::MsgToDpm_SrcCapsReceived& msg);
    void on_receive(const pd::MsgToDpm_SelectCapDone& msg);
    void on_receive(const pd::MsgToDpm_SrcDisabled& msg);
    void on_receive(const pd::MsgToDpm_Alert& msg);
    void on_receive(const pd::MsgToDpm_SnkReady& msg);
    void on_receive(const pd::MsgToDpm_CableDetached& msg);
    void on_receive(const pd::MsgToDpm_HandshakeDone& msg);
    void on_receive(const pd::MsgToDpm_NewPowerLevelRejected& msg);
    void on_receive_unknown(const etl::imessage& msg);
private:
    Power& power;
};

class Power: public etl::fsm {
public:
    static constexpr uint32_t UNKNOWN_RESISTANCE = etl::numeric_limits<uint32_t>::max();

    Power();
    void setup();
    void log_state();
    void log_unknown_event(const etl::imessage& msg);
    void log_pdos();

    void receive(const etl::imessage& message) {
        lock();
        etl::fsm::receive(message);
        unlock();
    }
    void transition_to(etl::fsm_state_id_t state) {
        lock();
        etl::fsm::transition_to(state);
        unlock();
    }

    // For external use only. Inside FSM use direct access
    // to avoid recursive lock.
    void set_power_mw(uint32_t mw) {
        lock();
        profile_selector.set_target_power_mw(mw);
        unlock();
    }

    uint32_t get_peak_mv();
    uint32_t get_peak_ma();
    uint32_t get_duty_x1000();
    uint32_t get_load_mohm();
    uint32_t get_max_power_mw();
    PowerStatus get_power_status() { return power_status; }
    void set_power_status(PowerStatus status) { power_status = status; }

    DPM_EventListener dpm_event_listener{*this};
    pd::PDO_LIST source_caps{};
    bool is_apdo_updating{false};
    bool is_from_caps_update{false};
    uint32_t prev_apdo_mv{0};
    uint32_t next_apdo_mv{0};

    Pwm pwm{};

    void lock() { xSemaphoreTake(_lock, portMAX_DELAY); }
    void unlock() { xSemaphoreGive(_lock); }

private:
    etl::atomic<PowerStatus> power_status{PowerStatus_PwrOff};
    SemaphoreHandle_t _lock{xSemaphoreCreateMutex()};
};

extern Power power;
