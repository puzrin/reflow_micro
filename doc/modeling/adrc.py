class ADRC:
    def __init__(self, b0, beta1, beta2, kp):
        self.b0 = b0
        self.beta1 = beta1
        self.beta2 = beta2
        self.kp = kp
        self.z1 = 0.0
        self.z2 = 0.0

    def update(self, y, y_ref, u_max, dt):
        # ESO updates
        e = y - self.z1
        self.z1 += dt * (self.z2 + self.beta1 * e)
        self.z2 += dt * (self.beta2 * e)
        
        # Control signal calculation
        u = -self.z2 / self.b0 + self.kp * (y_ref - self.z1)
        
        # Anti-windup [0, u_max]
        u_output = u
        if u > u_max: u_output = u_max
        if u < 0: u_output = 0
        
        # Adjust ESO to consider the actual control signal
        self.z2 += (u - u_output) * self.b0

        # Debugging output
        print(f"e: {e:.4f}, z1: {self.z1:.4f}, z2: {self.z2:.4f}, u: {u:.4f}, u_output: {u_output:.4f}")

        return u_output
    
    def reset_to(self, y):
        self.z1 = y
        self.z2 = 0.0
