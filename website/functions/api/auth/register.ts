import { hashPassword, createJwt, json, type Env } from '../_utils';

interface RequestBody { email: string; password: string; name?: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const { email, password, name } = await context.request.json<RequestBody>();
    if (!email || !password) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);

    const existing = await context.env.DB.prepare('SELECT id FROM users WHERE email = ?').bind(email).first();
    if (existing) return json({ status: 'error', code: 'EMAIL_EXISTS' }, 409);

    const id = crypto.randomUUID();
    const hashedPw = await hashPassword(password);

    await context.env.DB.prepare(
      'INSERT INTO users (id, email, password, name) VALUES (?, ?, ?, ?)'
    ).bind(id, email.toLowerCase(), hashedPw, name || null).run();

    const token = await createJwt({ userId: id, email: email.toLowerCase() }, context.env.JWT_SECRET);

    return json({ status: 'ok', user: { id, email } }, 201, {
      'Set-Cookie': `token=${token}; Path=/; HttpOnly; SameSite=Lax; Max-Age=${7 * 86400}`,
    });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
