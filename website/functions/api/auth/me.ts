import { getUserFromRequest, json, type Env } from '../_utils';

export const onRequestGet: PagesFunction<Env> = async (context) => {
  const tokenUser = await getUserFromRequest(context.request, context.env);
  if (!tokenUser) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

  const stmt = context.env.DB.prepare('SELECT id, email, name, created_at FROM users WHERE id = ?').bind(tokenUser.id);
  const user = await stmt.first();
  if (!user) return json({ status: 'error', code: 'USER_NOT_FOUND' }, 404);

  return json({ status: 'ok', user });
};
