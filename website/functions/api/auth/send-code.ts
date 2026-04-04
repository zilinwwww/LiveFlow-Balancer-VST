import { json, type Env } from '../_utils';

interface RequestBody { email: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const { email } = await context.request.json<RequestBody>();
    if (!email) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);
    const targetEmail = email.toLowerCase();

    // 1. Check if email already registered
    const existing = await context.env.DB.prepare('SELECT id FROM users WHERE email = ?').bind(targetEmail).first();
    if (existing) return json({ status: 'error', code: 'EMAIL_EXISTS' }, 409);

    // 2. Generate safe 6-digit alphanum code
    const code = Math.random().toString(36).substring(2, 8).toUpperCase();
    
    // 3. Store code in D1 with a 10 min expiration
    const expiresAt = Math.floor(Date.now() / 1000) + (10 * 60);

    // We use REPLACE INTO or INSERT OR REPLACE since email is primary key.
    await context.env.DB.prepare(
      'INSERT OR REPLACE INTO verification_codes (email, code, expires_at) VALUES (?, ?, ?)'
    ).bind(targetEmail, code, expiresAt).run();

    // 4. Send email
    // For local dev without API key: just log it
    if (!context.env.RESEND_API_KEY) {
      console.log(`[MOCK EMAIL] Verification code for ${targetEmail} is: ${code}`);
      return json({ status: 'ok', mocked: true });
    }

    const htmlBody = `
      <div style="font-family: sans-serif; padding: 20px;">
        <h2>LiveFlow Registration</h2>
        <p>Your verification code is: <strong style="font-size: 24px;">${code}</strong></p>
        <p>This code will expire in 10 minutes.</p>
        <p>If you didn't request this, you can safely ignore this email.</p>
      </div>
    `;

    const res = await fetch('https://api.resend.com/emails', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${context.env.RESEND_API_KEY}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        from: 'LiveFlow <noreply@micro-grav.com>', // Assuming default verified domain
        to: targetEmail,
        subject: 'LiveFlow Verification Code',
        html: htmlBody
      })
    });

    if (!res.ok) {
      const err = await res.text();
      console.error('Resend Error:', err);
      return json({ status: 'error', code: 'EMAIL_FAILED', details: err }, 500);
    }

    return json({ status: 'ok' });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
