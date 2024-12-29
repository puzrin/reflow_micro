#pragma once

#include <algorithm>
#include <cmath>

class ADRC {
private:
    float b0{0.0f};
    float beta1{0.0f};
    float beta2{0.0f};
    float kp{0.0f};
    float z1{0.0f};
    float z2{0.0f};

public:
    void set_params(float b0, float tau, float N, float M) {
        float omega_o = N / tau;
        float omega_c = omega_o / M;
        float Kp = omega_c / b0;
        set_params_raw(b0, omega_o, Kp);
    }

    void set_params_raw(float b0, float omega_o, float kp) {
        this->b0 = b0;
        this->beta1 = 2 * omega_o;
        this->beta2 = std::pow(omega_o, 2);
        this->kp = kp;
    }

    float iterate(float y, float y_ref, float u_max, float dt) {
        float e = y_ref - z1;
        float u = (kp * e - z2) / b0;

        // Anti-windup [0, u_max]
        float u_output = std::max(0.0f, std::min(u, u_max));

        // ESO update, with respect to real output
        float e_obs = y - z1;
        z1 += dt * (b0 * u_output + z2 + beta1 * e_obs);
        z2 += dt * (beta2 * e_obs);

        return u_output;
    }

    void reset_to(float y) {
        z1 = y;
        z2 = 0.0f;
    }
};