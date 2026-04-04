import { getUserFromRequest, json, type Env } from '../_utils';

export const onRequestGet: PagesFunction<Env> = async (context) => {
  try {
    const user = await getUserFromRequest(context.request, context.env);
    if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

    const { results } = await context.env.DB.prepare(
      'SELECT id, product, key, status, machine_id, machine_name, machine_info, bound_at, created_at FROM licenses WHERE user_id = ? ORDER BY created_at DESC'
    ).bind(user.id).all();

    return json({ status: 'ok', licenses: results });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
