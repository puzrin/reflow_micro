function hex2bytes(hex: string): Uint8Array {
    return new Uint8Array(hex.match(/.{1,2}/g)!.map(byte => parseInt(byte, 16)));
}
  
function bytes2hex(buffer: ArrayBuffer): string {
    return Array.from(new Uint8Array(buffer)).map(b => b.toString(16).padStart(2, '0')).join('');
}


export class AuthStorage {
    private client_id_key = 'rft_client_id';
    private secrets_key = 'rft_hmac_secrets';
  
    private getSecrets(): Record<string, string> {
      const data = localStorage.getItem(this.secrets_key);
      try {
        const obj = data ? JSON.parse(data) : {};

        if (obj && typeof obj === 'object' && !Array.isArray(obj)) return obj;
        return {};
      } catch {
        return {};
      }
    }
    
    public getSecret(device_id: string): string {
      const secrets = this.getSecrets();
      return secrets[device_id] || '';
    }
  
    public setSecret(device_id: string, secret: string): void {
      const secrets = this.getSecrets();

      if (!secret) delete secrets[device_id];
      else secrets[device_id] = secret

      localStorage.setItem(this.secrets_key, JSON.stringify(secrets));
    }
  
    public getClientId(): string {
        const key = 'rft_client_id';
        let clientId = localStorage.getItem(key);
    
        if (!clientId || typeof clientId !== 'string' || clientId.length !== 32) {
            const bytes = new Uint8Array(16);
            crypto.getRandomValues(bytes);
            clientId = Array.from(bytes).map(byte => byte.toString(16).padStart(2, '0')).join('');
            localStorage.setItem(key, clientId);
        }
    
        return clientId;
    }

    public async calculateHMAC(messageHex: string, keyHex: string): Promise<string> {
        const key = hex2bytes(keyHex);
        const message = hex2bytes(messageHex);
        const cryptoKey = await crypto.subtle.importKey('raw', key, { name: 'HMAC', hash: 'SHA-256' }, false, ['sign']);
        const signature = await crypto.subtle.sign('HMAC', cryptoKey, message);
        return bytes2hex(signature);
    }
}
