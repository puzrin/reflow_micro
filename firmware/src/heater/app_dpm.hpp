#pragma once

#include <pd/pd.h>

class AppDPM : public pd::DPM {
public:
    AppDPM(pd::Port& port, class Power& power) : pd::DPM(port), power(power) {}
    void setup() override;
    void clear_trigger_to(uint32_t pos, uint32_t mv);

private:
    Power& power;
};
