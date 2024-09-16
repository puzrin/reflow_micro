class PIDController:
    def __init__(self, Kp, Ki, Kd, setpoint, output_limits=(None, None), cut_with_derivative=False):
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.setpoint = setpoint
        
        # Ensure output limits are properly set
        if output_limits[0] is None or output_limits[1] is None:
            raise ValueError("Both output_limits must be defined.")
        
        self.output_min, self.output_max = output_limits
        self.integral = 0
        self.prev_error = 0
        self.cut_with_derivative = cut_with_derivative
        
    def reset(self):
        self.integral = 0
        self.prev_error = 0
        
    def compute(self, feedback_value, dt):
        error = self.setpoint - feedback_value
        delta_error = error - self.prev_error

        P = self.Kp * error
        I = self.Ki * self.integral
        D = self.Kd * delta_error / dt if dt > 0 else 0

        if self.cut_with_derivative: u_unsat = P + I + D
        else: u_unsat = P + I

        # Integral correction based on the unsaturated control signal
        if u_unsat > self.output_max: self.integral -= (u_unsat - self.output_max) / self.Ki
        if u_unsat < self.output_min: self.integral -= (u_unsat - self.output_min) / self.Ki
        
        # Recalculate the output after integral correction
        I = self.Ki * self.integral
        output = P + I + D

        # Manual clipping
        if output > self.output_max: output = self.output_max
        if output < self.output_min: output = self.output_min

        self.prev_error = error
        
        return output
