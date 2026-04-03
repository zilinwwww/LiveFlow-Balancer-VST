import { verifyPassword, createJwt, json, type Env } from '../_utils';

interface RequestBody { email: string; password: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const { email, password } = await context.request.json<RequestBody>();
    if (!email || !password) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);

    const user = await context.env.DB.prepare(
      'SELECT id, email, password FROM users WHERE email = ?'
    ).bind(email.toLowerCase()).first<{ id: string; email: string; password: string }>();

    if (!user) return json({ status: 'error', code: 'INVALID_CREDENTIALS' }, 401);

    const valid = await verifyPassword(password, user.password);
    if (!valid) return json({ status: 'error', code: 'INVALID_CREDENTIALS' }, 401);

    const token = await createJwt({ userId: user.id, email: user.email }, context.env.JWT_SECRET);

    return json({ status: 'ok', user: { id: user.id, email: user.email } }, 200, {
      'Set-Cookie': `token=${token}; Path=/; HttpOnly; SameSite=Strict; Max-Age=${7 * 86400}`,
    });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
