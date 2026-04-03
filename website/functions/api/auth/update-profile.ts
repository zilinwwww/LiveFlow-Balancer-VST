import { getUserFromRequest, json, type Env } from '../_utils';

export const onRequestPut: PagesFunction<Env> = async (context) => {
  const user = await getUserFromRequest(context.request, context.env);
  if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

  try {
    const { name } = (await context.request.json()) as { name?: string };
    
    await context.env.DB.prepare('UPDATE users SET name = ? WHERE id = ?')
      .bind(name || null, user.id)
      .run();

    return json({ status: 'ok' });
  } catch (e) {
    return json({ status: 'error', message: 'Internal error' }, 500);
  }
};
