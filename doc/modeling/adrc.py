# First order
class ADRC:
    def __init__(self, b0, ω_o, kp):
        self.b0 = b0
        self.beta1 = 2 * ω_o
        self.beta2 = ω_o ** 2
        self.kp = kp
        self.z1 = 0.0
        self.z2 = 0.0

    def iterate(self, y, y_ref, u_max, dt):
        # Control signal update
        e = y_ref - self.z1
        u = (self.kp * e - self.z2) / self.b0

        # Anti-windup [0, u_max]
        u_output = max(0, min(u, u_max))

        # ESO update, with respect to real output
        e_obs = y - self.z1
        self.z1 += dt * (self.b0 * u_output + self.z2 + self.beta1 * e_obs)
        self.z2 += dt * (self.beta2 * e_obs)

        return u_output

    def reset_to(self, y):
        self.z1 = y
        self.z2 = 0.0

# Second order
class ADRC2:
    def __init__(self, b0, ω_o, kp):
        self.b0 = b0
        self.beta1 = 3 * ω_o
        self.beta2 = 3 * ω_o ** 2
        self.beta3 = ω_o ** 3
        self.kp = kp
        self.z1 = 0.0
        self.z2 = 0.0
        self.z3 = 0.0

    def iterate(self, y, y_ref, u_max, dt):
        # Control signal update
        e = y_ref - self.z1
        u = (self.kp * e - self.z2 - self.z3) / self.b0

        # Anti-windup [0, u_max]
        u_output = max(0, min(u, u_max))

        # ESO update, with respect to real output
        e_obs = y - self.z1
        self.z1 += dt * (self.z2 + self.beta1 * e_obs)
        self.z2 += dt * (self.b0 * u_output + self.z3 + self.beta2 * e_obs)
        self.z3 += dt * (self.beta3 * e_obs)

        return u_output

    def reset_to(self, y):
        self.z1 = y
        self.z2 = 0.0
        self.z3 = 0.0
