#include <etl/limits.h>
#include <pd/pd.h>

#include "app.hpp"
#include "app_dpm.hpp"
#include "logger.hpp"
#include "power.hpp"

pd::Port port;
Power power;

pd::fusb302::Fusb302RtosHalEsp32 fusb302_hal;
pd::fusb302::Fusb302Rtos driver{port, fusb302_hal};

pd::Task task{port, driver};
AppDPM dpm{port, power};
pd::PRL prl{port, driver};
pd::PE pe{port, dpm, prl, driver};
pd::TC tc{port, driver};

namespace PWR_STATE {
    enum {
        Off,
        Initializing,
        Calibrate,
        Ready,
        WaitContractChange,
        Fault
    };
}

namespace {
    constexpr auto pwr_state_to_desc(int state) -> const char* {
        switch (state) {
            case PWR_STATE::Off: return "Off";
            case PWR_STATE::Initializing: return "Initializing";
            case PWR_STATE::Calibrate: return "Calibrate";
            case PWR_STATE::Ready: return "Ready";
            case PWR_STATE::WaitContractChange: return "WaitContractChange";
            case PWR_STATE::Fault: return "Fault";
            default: return "Unknown";
        }
    }
}

#define HANDLE_UNKNOWN_EVENT \
    auto on_event_unknown(const etl::imessage& event) -> etl::fsm_state_id_t { \
        return No_State_Change; \
    }


class PowerOff_State : public etl::fsm_state<Power, PowerOff_State, PWR_STATE::Off> {
public:
    HANDLE_UNKNOWN_EVENT;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        auto& pwr = get_fsm_context();
        pwr.log_state();

        pwr.pwm.enable(false);
        pwr.pwm.set_consumer_info(0, 0, false);
        pwr.profile_selector.set_target_power_mw(0);
        pwr.set_power_status(PowerStatus::PowerStatus_PwrOff);
        application.enqueue_message(AppCmd::Stop{});
        return No_State_Change;
    }
};

class PowerInitializing_State : public etl::fsm_state<Power, PowerInitializing_State, PWR_STATE::Initializing,
    pd::MsgToDpm_HandshakeDone, pd::MsgToDpm_SnkReady> {
public:
    HANDLE_UNKNOWN_EVENT;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        auto& pwr = get_fsm_context();
        pwr.log_state();

        auto& ps = pwr.profile_selector;
        dpm.clear_trigger_to(ps.default_position, ps.default_mv);

        pwr.pwm.enable(false);
        pwr.pwm.set_consumer_info(0, 0, false);
        pwr.profile_selector.set_target_power_mw(0);
        pwr.set_power_status(PowerStatus::PowerStatus_PwrInitializing);
        application.enqueue_message(AppCmd::Stop{});
        return No_State_Change;
    }

    auto on_event(const pd::MsgToDpm_HandshakeDone&) -> etl::fsm_state_id_t {
        return PWR_STATE::Calibrate;
    }

    auto on_event(const pd::MsgToDpm_SnkReady&) -> etl::fsm_state_id_t {
        if (port.pe_flags.test(pd::PE_FLAG::HANDSHAKE_REPORTED)) {
            return PWR_STATE::Calibrate;
        }
        return No_State_Change;
    }
};

class PowerCalibrate_State : public etl::fsm_state<Power, PowerCalibrate_State, PWR_STATE::Calibrate,
    MsgToPower_SysTick> {
public:
    HANDLE_UNKNOWN_EVENT;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        auto& pwr = get_fsm_context();
        pwr.log_state();

        auto ci = pwr.pwm.get_consumer_info();

        if (ci.is_actual && pwr.is_consumer_valid(ci)) {
            return PWR_STATE::Ready;
        }

        pwr.pwm.set_duty_x1000(0);
        pwr.pwm.enable(true);
        return No_State_Change;
    }

    auto on_event(const MsgToPower_SysTick&) -> etl::fsm_state_id_t {
        auto& pwr = get_fsm_context();

        auto ci = pwr.pwm.get_consumer_info();

        if (ci.is_actual && pwr.is_consumer_valid(ci)) {
            return PWR_STATE::Ready;
        }

        return No_State_Change;
    }
};

class PowerReady_State : public etl::fsm_state<Power, PowerReady_State, PWR_STATE::Ready,
    MsgToPower_SysTick, pd::MsgToDpm_SnkReady, pd::MsgToDpm_NewPowerLevelRejected> {
public:
    HANDLE_UNKNOWN_EVENT;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        auto& pwr = get_fsm_context();
        pwr.log_state();

        pwr.set_power_status(PowerStatus::PowerStatus_PwrOK);
        // Reset lock for sure
        pwr.is_apdo_updating = false;
        pwr.prev_apdo_mv = 0;
        // Don't start PWM/Profile here, wait for SysTick to kick in.
        // This will cause small delay on first entry, but that's acceptable.
        return No_State_Change;
    }

    auto on_event(const pd::MsgToDpm_NewPowerLevelRejected&) -> etl::fsm_state_id_t {
        auto& pwr = get_fsm_context();
        auto& ps = pwr.profile_selector;

        // Local APDO update failed
        pwr.is_apdo_updating = false;
        // Force re-init on failure
        // NOTE: trigger function MUST be async to avoid deadlock
        dpm.trigger_by_position(ps.default_position, ps.default_mv);
        return PWR_STATE::Initializing;
    }

    auto on_event(const pd::MsgToDpm_SnkReady&) -> etl::fsm_state_id_t {
        auto& pwr = get_fsm_context();

        // Local APDO update complete, and SRC is ready.
        // PWM stays intact, because been updated prior to APDO adjusting.
        pwr.prev_apdo_mv = pwr.next_apdo_mv;
        pwr.is_apdo_updating = false;
        return No_State_Change;
    }

    auto on_event(const MsgToPower_SysTick&) -> etl::fsm_state_id_t {
        auto& pwr = get_fsm_context();
        auto& ps = pwr.profile_selector;

        auto ci = pwr.pwm.get_consumer_info();
        auto consumer_valid = pwr.is_consumer_valid(ci);

        if (consumer_valid) {
            // Instant update profile selector with current load.
            // Here we don't check data actuality, because can arrive only
            // from 2 states - calibration & profile update. Saving a bit
            // outdated data is acceptable.
            //
            pwr.profile_selector.set_load_mohms(ci.peak_mv * 1000 / ci.peak_ma);
        }

        if (pwr.is_apdo_updating) {
            // Wait until APDO update is finished
            return No_State_Change;
        }

        if (!consumer_valid) {
            // If head connection lost for some reasons (ejected)
            pwr.pwm.enable(false);
            // NOTE: trigger function MUST be async to avoid deadlock
            dpm.trigger_by_position(ps.default_position, ps.default_mv);
            return PWR_STATE::Initializing;
        }

        // If completely new PDO required - go to switching state.
        if (pwr.profile_selector.better_pdo_available()) {
            APP_LOGI("Power: switch to better PDO (position {})", pwr.profile_selector.better_index + 1);
            return PWR_STATE::WaitContractChange;
        }

        auto idx = pwr.profile_selector.current_index;
        auto desc = pwr.profile_selector.descriptors[idx];
        auto params = pwr.profile_selector.get_pdo_usage_params(idx);

        if (desc.mv_min == desc.mv_max) {
            // Fixed PDO. Voltage is the same, only adjust duty cycle
            pwr.pwm.set_duty_x1000(params.duty_x1000);
            // Enable PWM if was inactive.
            // It's safe to call this multiple times
            pwr.pwm.enable(true);
        } else {
            // APDO
            pwr.pwm.set_duty_x1000(params.duty_x1000);
            pwr.pwm.enable(true);
            // Update APDO contract without state change. Lock next ticks
            // until update finishes.

            //
            // Avoid unnecessary APDO updates if voltage didn't change much
            //
            auto mv = (params.mv + 50) / 100 * 100; // Round tp 0.1V precision
            if (mv != pwr.prev_apdo_mv) {
                pwr.next_apdo_mv = mv;
                //APP_LOGI("Power: prev mV [{}], next mV [{}]", pwr.prev_apdo_mv, pwr.next_apdo_mv);

                // Note, state can be terminated from outside to init/off only.
                // The means stack was reset/disabled and no pending PS_RDY
                // will be left.
                pwr.is_apdo_updating = true;
                // NOTE: trigger function MUST be async to avoid deadlock
                dpm.trigger_by_position(idx + 1, params.mv);
            }
        }

        return No_State_Change;
    }
};

// Loopback from Ready state to wait for contract change after power update
class PowerWaitContractChange_State : public etl::fsm_state<Power, PowerWaitContractChange_State, PWR_STATE::WaitContractChange,
    pd::MsgToDpm_SnkReady, pd::MsgToDpm_NewPowerLevelRejected> {
public:
    HANDLE_UNKNOWN_EVENT;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        auto& pwr = get_fsm_context();
        pwr.log_state();

        pwr.set_power_status(PowerStatus::PowerStatus_PwrTransition);
        // Turn load off
        pwr.pwm.enable(false);
        // Initiate profile change
        auto idx = pwr.profile_selector.better_index;
        auto params = pwr.profile_selector.get_pdo_usage_params(idx);
        // NOTE: trigger function MUST be async to avoid deadlock
        dpm.trigger_by_position(idx + 1, params.mv);

        return No_State_Change;
    }

    auto on_event(const pd::MsgToDpm_NewPowerLevelRejected&) -> etl::fsm_state_id_t {
        auto& ps = get_fsm_context().profile_selector;
        // Force re-init on failure
        // NOTE: trigger function MUST be async to avoid deadlock
        dpm.trigger_by_position(ps.default_position, ps.default_mv);
        return PWR_STATE::Initializing;
    }

    auto on_event(const pd::MsgToDpm_SnkReady&) -> etl::fsm_state_id_t {
        return PWR_STATE::Ready;
    }
};

class PowerFault_State : public etl::fsm_state<Power, PowerFault_State, PWR_STATE::Fault> {
public:
    HANDLE_UNKNOWN_EVENT;

    auto on_enter_state() -> etl::fsm_state_id_t override {
        auto& pwr = get_fsm_context();
        pwr.log_state();

        pwr.pwm.enable(false);
        pwr.set_power_status(PowerStatus::PowerStatus_PwrFailure);
        return No_State_Change;
    }
};

etl::fsm_state_pack<
    PowerOff_State,
    PowerInitializing_State,
    PowerCalibrate_State,
    PowerReady_State,
    PowerWaitContractChange_State,
    PowerFault_State
> power_states;

Power::Power() : etl::fsm(0) {}

void Power::setup() {
    pwm.setup();
    set_states(power_states);
    start();
    task.start(tc, dpm, pe, prl, driver);
}

void Power::log_state() {
    APP_LOGD("Power: state => {}", pwr_state_to_desc(get_state_id()));
}

void Power::log_unknown_event(const etl::imessage& msg) {
    APP_LOGD("Power: Unknown event! msg id [{}], state id [{}]", msg.get_message_id(), get_state_id());
}

uint32_t Power::get_peak_mv() {
    auto ci = pwm.get_consumer_info();
    return ci.peak_mv;
}

uint32_t Power::get_peak_ma() {
    auto ci = pwm.get_consumer_info();
    return ci.peak_ma;
}

uint32_t Power::get_duty_x1000() {
    return pwm.get_duty_x1000();
}

uint32_t Power::get_load_mohm() {
    auto ci = pwm.get_consumer_info();

    if (!is_consumer_valid(ci)) { return etl::numeric_limits<uint32_t>::max(); }
    return ci.peak_mv * 1000 / ci.peak_ma;
}

uint32_t Power::get_max_power_mw() {
    auto ci = pwm.get_consumer_info();

    if (!is_consumer_valid(ci)) { return 0; }
    if (profile_selector.descriptors.size() == 0) { return 0; }

    profile_selector.set_load_mohms(ci.peak_mv * 1000 / ci.peak_ma);
    return profile_selector.mw_max(profile_selector.current_index);
}

bool Power::is_consumer_valid(const Pwm::CONSUMER_INFO& ci) {
    // These are thresholds for real measurements, with possible dropdowns.
    // Don't confuse with PD limits.
    return ci.peak_ma >= 300 && ci.peak_mv >= 4000;
}

void Power::log_pdos() {
    APP_LOGI("Power: Source capabilities received [{}]", source_caps.size());

    using namespace pd::dobj_utils;

    for (int i = 0; i < source_caps.size(); i++) {
        auto pdo = source_caps[i];

        if (pdo == 0) {
            APP_LOGD("  PDO[{}]: <PLACEHOLDER> (zero)", i+1);
            continue;
        }

        auto id = get_src_pdo_variant(pdo);

        if (id == pd::PDO_VARIANT::UNKNOWN) {
            APP_LOGD("  PDO[{}]: 0x{:08X} <UNKNOWN>", i+1, pdo);
        }
        else if (id == pd::PDO_VARIANT::FIXED) {
            ETL_MAYBE_UNUSED auto limits = get_src_pdo_limits(pdo);
            APP_LOGD("  PDO[{}]: 0x{:08X} <FIXED> {}mV {}mA",
                i+1, pdo, limits.mv_min, limits.ma);
        }
        else if (id == pd::PDO_VARIANT::APDO_PPS) {
            ETL_MAYBE_UNUSED auto limits = get_src_pdo_limits(pdo);
            APP_LOGD("  PDO[{}]: 0x{:08X} <APDO_PPS> {}-{}mV {}mA",
                i+1, pdo, limits.mv_min, limits.mv_max, limits.ma);
        }
        else if (id == pd::PDO_VARIANT::APDO_SPR_AVS) {
            ETL_MAYBE_UNUSED auto limits = get_src_pdo_limits(pdo);
            APP_LOGD("  PDO[{}]: 0x{:08X} <APDO_SPR_AVS> {}-{}mV {}mA",
                i+1, pdo, limits.mv_min, limits.mv_max, limits.ma);
        }
        else if (id == pd::PDO_VARIANT::APDO_EPR_AVS) {
            ETL_MAYBE_UNUSED auto limits = get_src_pdo_limits(pdo);
            APP_LOGD("  PDO[{}]: 0x{:08X} <APDO_EPR_AVS> {}-{}mV {}W",
                i+1, pdo, limits.mv_min, limits.mv_max, limits.pdp);
        }
        else {
            APP_LOGD("  PDO[{}]: 0x{:08X} <!!!UNHANDLED!!!>", i+1, pdo);
        }
    }
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_Startup&) {
    APP_LOGD("Power: PD Startup");
    power.transition_to(PWR_STATE::Initializing);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_TransitToDefault&) {
    APP_LOGD("Power: PD Transit to default");
    power.transition_to(PWR_STATE::Initializing);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_SrcCapsReceived&) {
    // Use shadow copy for eventual consistency
    power.lock();

    power.source_caps = port.source_caps;
    power.profile_selector.load_pdos(power.source_caps);
    auto & ps = power.profile_selector;
    dpm.clear_trigger_to(ps.default_position, ps.default_mv);

    power.unlock();

    power.log_pdos();
    // Re-init if charger forced caps emit (in normal case we are already there).
    // This should not happen, but may be possible on overload and so on.
    power.transition_to(PWR_STATE::Initializing);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_SelectCapDone&) {
    pd::RDO_ANY rdo{port.rdo_contracted};
    auto pdo_pos = rdo.obj_position;

    // APP_LOGD("===== Power: PD Select Cap done, position {}", pdo_pos);

    power.lock();
    power.profile_selector.set_pdo_index(pdo_pos - 1);
    power.unlock();
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_SrcDisabled&) {
    power.transition_to(PWR_STATE::Fault);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_Alert& msg) {
    APP_LOGE("Power: PD Alert [0x{:08X}]", msg.value);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_SnkReady& msg) {
    power.receive(msg);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_CableDetached& msg) {
    power.transition_to(PWR_STATE::Off);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_HandshakeDone& msg) {
    power.receive(msg);
}

void DPM_EventListener::on_receive(const pd::MsgToDpm_NewPowerLevelRejected& msg) {
    APP_LOGE("Power: PD new power level rejected (MsgToDpm_NewPowerLevelRejected)");
    power.receive(msg);
}

void DPM_EventListener::on_receive_unknown(const etl::imessage& msg) {
    //APP_LOGV("Power: Unknown msg (id={})", msg.get_message_id());
}
