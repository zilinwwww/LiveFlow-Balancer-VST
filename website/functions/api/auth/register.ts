import { hashPassword, createJwt, json, type Env } from '../_utils';

interface RequestBody { email: string; password: string; name?: string; code?: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const { email, password, name, code } = await context.request.json<RequestBody>();
    if (!email || !password || !code) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);

    const targetEmail = email.toLowerCase();
    const existing = await context.env.DB.prepare('SELECT id FROM users WHERE email = ?').bind(targetEmail).first();
    if (existing) return json({ status: 'error', code: 'EMAIL_EXISTS' }, 409);

    // Validate email verification code
    const verification = await context.env.DB.prepare('SELECT code, expires_at FROM verification_codes WHERE email = ?').bind(targetEmail).first<{ code: string; expires_at: number }>();
    if (!verification) return json({ status: 'error', code: 'CODE_INVALID' }, 400);
    
    // Check if code matches and is not expired
    const now = Math.floor(Date.now() / 1000);
    if (verification.code !== code || now > verification.expires_at) {
      return json({ status: 'error', code: 'CODE_INVALID' }, 400);
    }

    const id = crypto.randomUUID();
    const hashedPw = await hashPassword(password);

    await context.env.DB.prepare(
      'INSERT INTO users (id, email, password, name) VALUES (?, ?, ?, ?)'
    ).bind(id, targetEmail, hashedPw, name || null).run();

    // Cleanup used verification code
    await context.env.DB.prepare('DELETE FROM verification_codes WHERE email = ?').bind(targetEmail).run();

    const token = await createJwt({ userId: id, email: targetEmail }, context.env.JWT_SECRET);

    return json({ status: 'ok', user: { id, email: targetEmail } }, 201, {
      'Set-Cookie': `token=${token}; Path=/; HttpOnly; SameSite=Lax; Max-Age=${7 * 86400}`,
    });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
