// Due wish to keep BLE RPC self-suffucient, auth storage implemented
// "manually", instead of Pinia persistance.

function hex2bytes(hex: string): Uint8Array {
    return new Uint8Array(hex.match(/.{1,2}/g)!.map(byte => parseInt(byte, 16)));
}

function bytes2hex(buffer: ArrayBuffer): string {
    return Array.from(new Uint8Array(buffer)).map(b => b.toString(16).padStart(2, '0')).join('');
}


export class AuthStorage {
    private client_id_key = 'rft_client_id';
    private secrets_key = 'rft_hmac_secrets';

    private getSecretsObj(): Record<string, string> {
      const data = localStorage.getItem(this.secrets_key);
      try {
        const obj = data ? JSON.parse(data) : {};

        if (obj && typeof obj === 'object' && !Array.isArray(obj)) return obj;
        return {};
      } catch {
        return {};
      }
    }

    public hasSecret(device_id: Uint8Array) : boolean {
      const secrets = this.getSecretsObj();
      return !!secrets[bytes2hex(device_id)]
    }

    public getSecret(device_id: Uint8Array): Uint8Array {
      const secrets = this.getSecretsObj();
      return hex2bytes(secrets[bytes2hex(device_id)] || '');
    }

    public setSecret(device_id: Uint8Array, secret: Uint8Array | null): void {
      const secrets = this.getSecretsObj();
      const id = bytes2hex(device_id);

      if (!secret) delete secrets[id];
      else secrets[id] = bytes2hex(secret)

      localStorage.setItem(this.secrets_key, JSON.stringify(secrets));
    }

    public getClientId(): Uint8Array {
        const key = 'rft_client_id';
        let clientId = localStorage.getItem(key);

        if (!clientId || typeof clientId !== 'string' || clientId.length !== 32) {
            const bytes = new Uint8Array(16);
            crypto.getRandomValues(bytes);
            clientId = Array.from(bytes).map(byte => byte.toString(16).padStart(2, '0')).join('');
            localStorage.setItem(key, clientId);
        }

        return hex2bytes(clientId);
    }

    public async calculateHMAC(message: Uint8Array, key: Uint8Array): Promise<Uint8Array> {
        const cryptoKey = await crypto.subtle.importKey('raw', key, { name: 'HMAC', hash: 'SHA-256' }, false, ['sign']);
        const signature = await crypto.subtle.sign('HMAC', cryptoKey, message);
        return new Uint8Array(signature);
    }
}
