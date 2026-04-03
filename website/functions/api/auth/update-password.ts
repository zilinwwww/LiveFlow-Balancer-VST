import { getUserFromRequest, json, verifyPassword, hashPassword, type Env } from '../_utils';

export const onRequestPut: PagesFunction<Env> = async (context) => {
  const user = await getUserFromRequest(context.request, context.env);
  if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

  try {
    const { oldPassword, newPassword } = (await context.request.json()) as any;
    if (!oldPassword || !newPassword || newPassword.length < 8) {
      return json({ status: 'error', code: 'INVALID_INPUT' }, 400);
    }

    const stmt = context.env.DB.prepare('SELECT password FROM users WHERE id = ?').bind(user.id);
    const row = await stmt.first();
    if (!row) return json({ status: 'error', code: 'USER_NOT_FOUND' }, 404);

    const isValid = await verifyPassword(oldPassword, row.password as string);
    if (!isValid) return json({ status: 'error', code: 'INVALID_OLD_PASSWORD' }, 400);

    const newHash = await hashPassword(newPassword);
    await context.env.DB.prepare('UPDATE users SET password = ? WHERE id = ?')
      .bind(newHash, user.id)
      .run();

    return json({ status: 'ok' });
  } catch (e) {
    return json({ status: 'error', message: 'Internal error' }, 500);
  }
};
