// Shared crypto + JWT utilities for Cloudflare Workers (Web Crypto API)

export interface Env {
  DB: D1Database;
  JWT_SECRET: string;
}

// ── Password Hashing (PBKDF2) ──

export async function hashPassword(password: string): Promise<string> {
  const salt = crypto.getRandomValues(new Uint8Array(16));
  const keyMaterial = await crypto.subtle.importKey(
    'raw', new TextEncoder().encode(password), 'PBKDF2', false, ['deriveBits']
  );
  const hash = await crypto.subtle.deriveBits(
    { name: 'PBKDF2', salt, iterations: 100000, hash: 'SHA-256' },
    keyMaterial, 256
  );
  const saltHex = Array.from(salt).map(b => b.toString(16).padStart(2, '0')).join('');
  const hashHex = Array.from(new Uint8Array(hash)).map(b => b.toString(16).padStart(2, '0')).join('');
  return `${saltHex}:${hashHex}`;
}

export async function verifyPassword(password: string, stored: string): Promise<boolean> {
  const [saltHex, expectedHash] = stored.split(':');
  const salt = new Uint8Array(saltHex.match(/.{2}/g)!.map(b => parseInt(b, 16)));
  const keyMaterial = await crypto.subtle.importKey(
    'raw', new TextEncoder().encode(password), 'PBKDF2', false, ['deriveBits']
  );
  const hash = await crypto.subtle.deriveBits(
    { name: 'PBKDF2', salt, iterations: 100000, hash: 'SHA-256' },
    keyMaterial, 256
  );
  const hashHex = Array.from(new Uint8Array(hash)).map(b => b.toString(16).padStart(2, '0')).join('');
  return hashHex === expectedHash;
}

// ── JWT (HMAC-SHA256) ──

async function getJwtKey(secret: string): Promise<CryptoKey> {
  return crypto.subtle.importKey(
    'raw', new TextEncoder().encode(secret),
    { name: 'HMAC', hash: 'SHA-256' }, false, ['sign', 'verify']
  );
}

function base64UrlEncode(data: Uint8Array): string {
  return btoa(String.fromCharCode(...data))
    .replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}

function base64UrlDecode(str: string): Uint8Array {
  const padded = str.replace(/-/g, '+').replace(/_/g, '/');
  const binary = atob(padded);
  return new Uint8Array([...binary].map(c => c.charCodeAt(0)));
}

export async function createJwt(payload: Record<string, unknown>, secret: string): Promise<string> {
  const header = { alg: 'HS256', typ: 'JWT' };
  const now = Math.floor(Date.now() / 1000);
  const fullPayload = { ...payload, iat: now, exp: now + 7 * 24 * 3600 }; // 7 days

  const headerB64 = base64UrlEncode(new TextEncoder().encode(JSON.stringify(header)));
  const payloadB64 = base64UrlEncode(new TextEncoder().encode(JSON.stringify(fullPayload)));
  const signingInput = `${headerB64}.${payloadB64}`;

  const key = await getJwtKey(secret);
  const signature = await crypto.subtle.sign('HMAC', key, new TextEncoder().encode(signingInput));

  return `${signingInput}.${base64UrlEncode(new Uint8Array(signature))}`;
}

export async function verifyJwt(token: string, secret: string): Promise<Record<string, unknown> | null> {
  const parts = token.split('.');
  if (parts.length !== 3) return null;

  const [headerB64, payloadB64, sigB64] = parts;
  const key = await getJwtKey(secret);
  const valid = await crypto.subtle.verify(
    'HMAC', key,
    base64UrlDecode(sigB64),
    new TextEncoder().encode(`${headerB64}.${payloadB64}`)
  );
  if (!valid) return null;

  const payload = JSON.parse(new TextDecoder().decode(base64UrlDecode(payloadB64)));
  if (payload.exp && payload.exp < Math.floor(Date.now() / 1000)) return null; // expired
  return payload;
}

// ── Auth Helper ──

export async function getUserFromRequest(request: Request, env: Env): Promise<{ id: string; email: string } | null> {
  const cookie = request.headers.get('Cookie') || '';
  const match = cookie.match(/token=([^;]+)/);
  if (!match) return null;

  const payload = await verifyJwt(match[1], env.JWT_SECRET);
  if (!payload || !payload.userId) return null;

  return { id: payload.userId as string, email: payload.email as string };
}

// ── License Key Generator ──

export function generateLicenseKey(): string {
  const chars = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'; // no ambiguous chars (0/O, 1/I)
  const group = () => Array.from(crypto.getRandomValues(new Uint8Array(4)))
    .map(b => chars[b % chars.length]).join('');
  return `LF-${group()}-${group()}-${group()}-${group()}`;
}

// ── JSON Response Helper ──

export function json(data: unknown, status = 200, headers: Record<string, string> = {}): Response {
  return new Response(JSON.stringify(data), {
    status,
    headers: { 'Content-Type': 'application/json', ...headers },
  });
}
