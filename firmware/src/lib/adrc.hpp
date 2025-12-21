#pragma once

#include <etl/algorithm.h>

class ADRC {
private:
    float b0{0.0F};
    float beta1{0.0F};
    float beta2{0.0F};
    float kp{0.0F};
    float z1{0.0F};
    float z2{0.0F};

public:
    void set_params(float b0, float tau, float N, float M) {
        const float omega_c = N / tau;
        const float omega_o = M * omega_c;
        const float Kp = omega_c;
        set_params_raw(b0, omega_o, Kp);
    }

    void set_params_raw(float b0, float omega_o, float kp) {
        this->b0 = b0;
        this->beta1 = 2 * omega_o;
        this->beta2 = powf(omega_o, 2);
        this->kp = kp;
    }

    auto iterate(float y, float y_ref, float u_max, float dt) -> float {
        const float e = y_ref - z1;
        const float u = (kp * e - z2) / b0;

        // Anti-windup [0, u_max]
        const float u_output = etl::max(0.0f, etl::min(u, u_max));

        // ESO update, with respect to real output
        const float e_obs = y - z1;
        z1 += dt * (b0 * u_output + z2 + beta1 * e_obs);
        z2 += dt * (beta2 * e_obs);

        return u_output;
    }

    void reset_to(float y) {
        z1 = y;
        z2 = 0.0F;
    }
};
