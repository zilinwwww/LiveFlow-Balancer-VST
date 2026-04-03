import { getUserFromRequest, json, type Env } from '../_utils';

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const user = await getUserFromRequest(context.request, context.env);
    if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

    const { licenseId } = await context.request.json<{ licenseId: string }>();
    if (!licenseId) return json({ status: 'error', code: 'MISSING_LICENSE_ID' }, 400);

    // Verify the license belongs to this user
    const license = await context.env.DB.prepare(
      'SELECT id, status FROM licenses WHERE id = ? AND user_id = ?'
    ).bind(licenseId, user.id).first<{ id: string; status: string }>();

    if (!license) return json({ status: 'error', code: 'NOT_FOUND' }, 404);
    if (license.status !== 'bound') return json({ status: 'error', code: 'NOT_BOUND' }, 400);

    await context.env.DB.prepare(
      "UPDATE licenses SET status = 'available', machine_id = NULL, bound_at = NULL WHERE id = ?"
    ).bind(licenseId).run();

    return json({ status: 'ok', message: 'License unbound successfully' });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
