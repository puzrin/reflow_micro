// First order
export class ADRC {
    private b0: number = 0.0;
    private beta1: number = 0.0;
    private beta2: number = 0.0;
    private kp: number = 0.0;
    private z1: number = 0.0;
    private z2: number = 0.0;

    set_params(b0: number, τ: number, N: number, M: number): void {
        const ω_c = N / τ
        const ω_o = M * ω_c
        const Kp = ω_c
        this.set_params_raw(b0, ω_o, Kp)
    }

    set_params_raw(b0: number, ω_o: number, kp: number): void {
        this.b0 = b0;
        this.beta1 = 2 * ω_o;
        this.beta2 = ω_o ** 2;
        this.kp = kp;
    }

    iterate(y: number, y_ref: number, u_max: number, dt: number): number {
        const e = y_ref - this.z1;
        const u = (this.kp * e - this.z2) / this.b0;

        // Anti-windup [0, u_max]
        const u_output = Math.max(0, Math.min(u, u_max));

        // ESO update, with respect to real output
        const e_obs = y - this.z1;
        this.z1 += dt * (this.b0 * u_output + this.z2 + this.beta1 * e_obs);
        this.z2 += dt * (this.beta2 * e_obs);

        return u_output;
    }

    reset_to(y: number): void {
        this.z1 = y;
        this.z2 = 0.0;
    }
}
