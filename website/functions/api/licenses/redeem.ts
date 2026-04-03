import { getUserFromRequest, json, type Env } from '../_utils';

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const user = await getUserFromRequest(context.request, context.env);
    if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

    const { key } = await context.request.json<{ key: string }>();
    if (!key) return json({ status: 'error', code: 'MISSING_KEY' }, 400);

    // Find a license with this key that has no owner (pre-provisioned)
    const license = await context.env.DB.prepare(
      'SELECT id, product, status, user_id FROM licenses WHERE key = ?'
    ).bind(key.trim().toUpperCase()).first<{ id: string; product: string; status: string; user_id: string | null }>();

    if (!license) return json({ status: 'error', code: 'INVALID_KEY' }, 404);
    if (license.user_id && license.user_id !== user.id) return json({ status: 'error', code: 'KEY_TAKEN' }, 409);
    if (license.user_id === user.id) return json({ status: 'error', code: 'ALREADY_OWNED' }, 409);

    // Assign the license to this user
    await context.env.DB.prepare(
      'UPDATE licenses SET user_id = ? WHERE id = ?'
    ).bind(user.id, license.id).run();

    return json({ status: 'ok', license: { id: license.id, product: license.product, key, status: license.status } }, 200);
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
